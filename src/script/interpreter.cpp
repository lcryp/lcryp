#include <script/interpreter.h>
#include <crypto/ripemd160.h>
#include <crypto/sha1.h>
#include <crypto/sha256.h>
#include <pubkey.h>
#include <script/script.h>
#include <uint256.h>
typedef std::vector<unsigned char> valtype;
namespace {
inline bool set_success(ScriptError* ret)
{
    if (ret)
        *ret = SCRIPT_ERR_OK;
    return true;
}
inline bool set_error(ScriptError* ret, const ScriptError serror)
{
    if (ret)
        *ret = serror;
    return false;
}
}
bool CastToBool(const valtype& vch)
{
    for (unsigned int i = 0; i < vch.size(); i++)
    {
        if (vch[i] != 0)
        {
            if (i == vch.size()-1 && vch[i] == 0x80)
                return false;
            return true;
        }
    }
    return false;
}
#define stacktop(i)  (stack.at(stack.size()+(i)))
#define altstacktop(i)  (altstack.at(altstack.size()+(i)))
static inline void popstack(std::vector<valtype>& stack)
{
    if (stack.empty())
        throw std::runtime_error("popstack(): stack empty");
    stack.pop_back();
}
bool static IsCompressedOrUncompressedPubKey(const valtype &vchPubKey) {
    if (vchPubKey.size() < CPubKey::COMPRESSED_SIZE) {
        return false;
    }
    if (vchPubKey[0] == 0x04) {
        if (vchPubKey.size() != CPubKey::SIZE) {
            return false;
        }
    } else if (vchPubKey[0] == 0x02 || vchPubKey[0] == 0x03) {
        if (vchPubKey.size() != CPubKey::COMPRESSED_SIZE) {
            return false;
        }
    } else {
        return false;
    }
    return true;
}
bool static IsCompressedPubKey(const valtype &vchPubKey) {
    if (vchPubKey.size() != CPubKey::COMPRESSED_SIZE) {
        return false;
    }
    if (vchPubKey[0] != 0x02 && vchPubKey[0] != 0x03) {
        return false;
    }
    return true;
}
bool static IsValidSignatureEncoding(const std::vector<unsigned char> &sig) {
    if (sig.size() < 9) return false;
    if (sig.size() > 73) return false;
    if (sig[0] != 0x30) return false;
    if (sig[1] != sig.size() - 3) return false;
    unsigned int lenR = sig[3];
    if (5 + lenR >= sig.size()) return false;
    unsigned int lenS = sig[5 + lenR];
    if ((size_t)(lenR + lenS + 7) != sig.size()) return false;
    if (sig[2] != 0x02) return false;
    if (lenR == 0) return false;
    if (sig[4] & 0x80) return false;
    if (lenR > 1 && (sig[4] == 0x00) && !(sig[5] & 0x80)) return false;
    if (sig[lenR + 4] != 0x02) return false;
    if (lenS == 0) return false;
    if (sig[lenR + 6] & 0x80) return false;
    if (lenS > 1 && (sig[lenR + 6] == 0x00) && !(sig[lenR + 7] & 0x80)) return false;
    return true;
}
bool static IsLowDERSignature(const valtype &vchSig, ScriptError* serror) {
    if (!IsValidSignatureEncoding(vchSig)) {
        return set_error(serror, SCRIPT_ERR_SIG_DER);
    }
    std::vector<unsigned char> vchSigCopy(vchSig.begin(), vchSig.begin() + vchSig.size() - 1);
    if (!CPubKey::CheckLowS(vchSigCopy)) {
        return set_error(serror, SCRIPT_ERR_SIG_HIGH_S);
    }
    return true;
}
bool static IsDefinedHashtypeSignature(const valtype &vchSig) {
    if (vchSig.size() == 0) {
        return false;
    }
    unsigned char nHashType = vchSig[vchSig.size() - 1] & (~(SIGHASH_ANYONECANPAY));
    if (nHashType < SIGHASH_ALL || nHashType > SIGHASH_SINGLE)
        return false;
    return true;
}
bool CheckSignatureEncoding(const std::vector<unsigned char> &vchSig, unsigned int flags, ScriptError* serror) {
    if (vchSig.size() == 0) {
        return true;
    }
    if ((flags & (SCRIPT_VERIFY_DERSIG | SCRIPT_VERIFY_LOW_S | SCRIPT_VERIFY_STRICTENC)) != 0 && !IsValidSignatureEncoding(vchSig)) {
        return set_error(serror, SCRIPT_ERR_SIG_DER);
    } else if ((flags & SCRIPT_VERIFY_LOW_S) != 0 && !IsLowDERSignature(vchSig, serror)) {
        return false;
    } else if ((flags & SCRIPT_VERIFY_STRICTENC) != 0 && !IsDefinedHashtypeSignature(vchSig)) {
        return set_error(serror, SCRIPT_ERR_SIG_HASHTYPE);
    }
    return true;
}
bool static CheckPubKeyEncoding(const valtype &vchPubKey, unsigned int flags, const SigVersion &sigversion, ScriptError* serror) {
    if ((flags & SCRIPT_VERIFY_STRICTENC) != 0 && !IsCompressedOrUncompressedPubKey(vchPubKey)) {
        return set_error(serror, SCRIPT_ERR_PUBKEYTYPE);
    }
    if ((flags & SCRIPT_VERIFY_WITNESS_PUBKEYTYPE) != 0 && sigversion == SigVersion::WITNESS_V0 && !IsCompressedPubKey(vchPubKey)) {
        return set_error(serror, SCRIPT_ERR_WITNESS_PUBKEYTYPE);
    }
    return true;
}
int FindAndDelete(CScript& script, const CScript& b)
{
    int nFound = 0;
    if (b.empty())
        return nFound;
    CScript result;
    CScript::const_iterator pc = script.begin(), pc2 = script.begin(), end = script.end();
    opcodetype opcode;
    do
    {
        result.insert(result.end(), pc2, pc);
        while (static_cast<size_t>(end - pc) >= b.size() && std::equal(b.begin(), b.end(), pc))
        {
            pc = pc + b.size();
            ++nFound;
        }
        pc2 = pc;
    }
    while (script.GetOp(pc, opcode));
    if (nFound > 0) {
        result.insert(result.end(), pc2, end);
        script = std::move(result);
    }
    return nFound;
}
namespace {
class ConditionStack {
private:
    static constexpr uint32_t NO_FALSE = std::numeric_limits<uint32_t>::max();
    uint32_t m_stack_size = 0;
    uint32_t m_first_false_pos = NO_FALSE;
public:
    bool empty() const { return m_stack_size == 0; }
    bool all_true() const { return m_first_false_pos == NO_FALSE; }
    void push_back(bool f)
    {
        if (m_first_false_pos == NO_FALSE && !f) {
            m_first_false_pos = m_stack_size;
        }
        ++m_stack_size;
    }
    void pop_back()
    {
        assert(m_stack_size > 0);
        --m_stack_size;
        if (m_first_false_pos == m_stack_size) {
            m_first_false_pos = NO_FALSE;
        }
    }
    void toggle_top()
    {
        assert(m_stack_size > 0);
        if (m_first_false_pos == NO_FALSE) {
            m_first_false_pos = m_stack_size - 1;
        } else if (m_first_false_pos == m_stack_size - 1) {
            m_first_false_pos = NO_FALSE;
        } else {
        }
    }
};
}
static bool EvalChecksigPreTapscript(const valtype& vchSig, const valtype& vchPubKey, CScript::const_iterator pbegincodehash, CScript::const_iterator pend, unsigned int flags, const BaseSignatureChecker& checker, SigVersion sigversion, ScriptError* serror, bool& fSuccess)
{
    assert(sigversion == SigVersion::BASE || sigversion == SigVersion::WITNESS_V0);
    CScript scriptCode(pbegincodehash, pend);
    if (sigversion == SigVersion::BASE) {
        int found = FindAndDelete(scriptCode, CScript() << vchSig);
        if (found > 0 && (flags & SCRIPT_VERIFY_CONST_SCRIPTCODE))
            return set_error(serror, SCRIPT_ERR_SIG_FINDANDDELETE);
    }
    if (!CheckSignatureEncoding(vchSig, flags, serror) || !CheckPubKeyEncoding(vchPubKey, flags, sigversion, serror)) {
        return false;
    }
    fSuccess = checker.CheckECDSASignature(vchSig, vchPubKey, scriptCode, sigversion);
    if (!fSuccess && (flags & SCRIPT_VERIFY_NULLFAIL) && vchSig.size())
        return set_error(serror, SCRIPT_ERR_SIG_NULLFAIL);
    return true;
}
static bool EvalChecksigTapscript(const valtype& sig, const valtype& pubkey, ScriptExecutionData& execdata, unsigned int flags, const BaseSignatureChecker& checker, SigVersion sigversion, ScriptError* serror, bool& success)
{
    assert(sigversion == SigVersion::TAPSCRIPT);
    success = !sig.empty();
    if (success) {
        assert(execdata.m_validation_weight_left_init);
        execdata.m_validation_weight_left -= VALIDATION_WEIGHT_PER_SIGOP_PASSED;
        if (execdata.m_validation_weight_left < 0) {
            return set_error(serror, SCRIPT_ERR_TAPSCRIPT_VALIDATION_WEIGHT);
        }
    }
    if (pubkey.size() == 0) {
        return set_error(serror, SCRIPT_ERR_PUBKEYTYPE);
    } else if (pubkey.size() == 32) {
        if (success && !checker.CheckSchnorrSignature(sig, pubkey, sigversion, execdata, serror)) {
            return false;
        }
    } else {
        if ((flags & SCRIPT_VERIFY_DISCOURAGE_UPGRADABLE_PUBKEYTYPE) != 0) {
            return set_error(serror, SCRIPT_ERR_DISCOURAGE_UPGRADABLE_PUBKEYTYPE);
        }
    }
    return true;
}
static bool EvalChecksig(const valtype& sig, const valtype& pubkey, CScript::const_iterator pbegincodehash, CScript::const_iterator pend, ScriptExecutionData& execdata, unsigned int flags, const BaseSignatureChecker& checker, SigVersion sigversion, ScriptError* serror, bool& success)
{
    switch (sigversion) {
    case SigVersion::BASE:
    case SigVersion::WITNESS_V0:
        return EvalChecksigPreTapscript(sig, pubkey, pbegincodehash, pend, flags, checker, sigversion, serror, success);
    case SigVersion::TAPSCRIPT:
        return EvalChecksigTapscript(sig, pubkey, execdata, flags, checker, sigversion, serror, success);
    case SigVersion::TAPROOT:
        break;
    }
    assert(false);
}
bool EvalScript(std::vector<std::vector<unsigned char> >& stack, const CScript& script, unsigned int flags, const BaseSignatureChecker& checker, SigVersion sigversion, ScriptExecutionData& execdata, ScriptError* serror)
{
    static const CScriptNum bnZero(0);
    static const CScriptNum bnOne(1);
    static const valtype vchFalse(0);
    static const valtype vchTrue(1, 1);
    assert(sigversion == SigVersion::BASE || sigversion == SigVersion::WITNESS_V0 || sigversion == SigVersion::TAPSCRIPT);
    CScript::const_iterator pc = script.begin();
    CScript::const_iterator pend = script.end();
    CScript::const_iterator pbegincodehash = script.begin();
    opcodetype opcode;
    valtype vchPushValue;
    ConditionStack vfExec;
    std::vector<valtype> altstack;
    set_error(serror, SCRIPT_ERR_UNKNOWN_ERROR);
    if ((sigversion == SigVersion::BASE || sigversion == SigVersion::WITNESS_V0) && script.size() > MAX_SCRIPT_SIZE) {
        return set_error(serror, SCRIPT_ERR_SCRIPT_SIZE);
    }
    int nOpCount = 0;
    bool fRequireMinimal = (flags & SCRIPT_VERIFY_MINIMALDATA) != 0;
    uint32_t opcode_pos = 0;
    execdata.m_codeseparator_pos = 0xFFFFFFFFUL;
    execdata.m_codeseparator_pos_init = true;
    try
    {
        for (; pc < pend; ++opcode_pos) {
            bool fExec = vfExec.all_true();
            if (!script.GetOp(pc, opcode, vchPushValue))
                return set_error(serror, SCRIPT_ERR_BAD_OPCODE);
            if (vchPushValue.size() > MAX_SCRIPT_ELEMENT_SIZE)
                return set_error(serror, SCRIPT_ERR_PUSH_SIZE);
            if (sigversion == SigVersion::BASE || sigversion == SigVersion::WITNESS_V0) {
                if (opcode > OP_16 && ++nOpCount > MAX_OPS_PER_SCRIPT) {
                    return set_error(serror, SCRIPT_ERR_OP_COUNT);
                }
            }
            if (opcode == OP_CAT ||
                opcode == OP_SUBSTR ||
                opcode == OP_LEFT ||
                opcode == OP_RIGHT ||
                opcode == OP_INVERT ||
                opcode == OP_AND ||
                opcode == OP_OR ||
                opcode == OP_XOR ||
                opcode == OP_2MUL ||
                opcode == OP_2DIV ||
                opcode == OP_MUL ||
                opcode == OP_DIV ||
                opcode == OP_MOD ||
                opcode == OP_LSHIFT ||
                opcode == OP_RSHIFT)
                return set_error(serror, SCRIPT_ERR_DISABLED_OPCODE);
            if (opcode == OP_CODESEPARATOR && sigversion == SigVersion::BASE && (flags & SCRIPT_VERIFY_CONST_SCRIPTCODE))
                return set_error(serror, SCRIPT_ERR_OP_CODESEPARATOR);
            if (fExec && 0 <= opcode && opcode <= OP_PUSHDATA4) {
                if (fRequireMinimal && !CheckMinimalPush(vchPushValue, opcode)) {
                    return set_error(serror, SCRIPT_ERR_MINIMALDATA);
                }
                stack.push_back(vchPushValue);
            } else if (fExec || (OP_IF <= opcode && opcode <= OP_ENDIF))
            switch (opcode)
            {
                case OP_1NEGATE:
                case OP_1:
                case OP_2:
                case OP_3:
                case OP_4:
                case OP_5:
                case OP_6:
                case OP_7:
                case OP_8:
                case OP_9:
                case OP_10:
                case OP_11:
                case OP_12:
                case OP_13:
                case OP_14:
                case OP_15:
                case OP_16:
                {
                    CScriptNum bn((int)opcode - (int)(OP_1 - 1));
                    stack.push_back(bn.getvch());
                }
                break;
                case OP_NOP:
                    break;
                case OP_CHECKLOCKTIMEVERIFY:
                {
                    if (!(flags & SCRIPT_VERIFY_CHECKLOCKTIMEVERIFY)) {
                        break;
                    }
                    if (stack.size() < 1)
                        return set_error(serror, SCRIPT_ERR_INVALID_STACK_OPERATION);
                    const CScriptNum nLockTime(stacktop(-1), fRequireMinimal, 5);
                    if (nLockTime < 0)
                        return set_error(serror, SCRIPT_ERR_NEGATIVE_LOCKTIME);
                    if (!checker.CheckLockTime(nLockTime))
                        return set_error(serror, SCRIPT_ERR_UNSATISFIED_LOCKTIME);
                    break;
                }
                case OP_CHECKSEQUENCEVERIFY:
                {
                    if (!(flags & SCRIPT_VERIFY_CHECKSEQUENCEVERIFY)) {
                        break;
                    }
                    if (stack.size() < 1)
                        return set_error(serror, SCRIPT_ERR_INVALID_STACK_OPERATION);
                    const CScriptNum nSequence(stacktop(-1), fRequireMinimal, 5);
                    if (nSequence < 0)
                        return set_error(serror, SCRIPT_ERR_NEGATIVE_LOCKTIME);
                    if ((nSequence & CTxIn::SEQUENCE_LOCKTIME_DISABLE_FLAG) != 0)
                        break;
                    if (!checker.CheckSequence(nSequence))
                        return set_error(serror, SCRIPT_ERR_UNSATISFIED_LOCKTIME);
                    break;
                }
                case OP_NOP1: case OP_NOP4: case OP_NOP5:
                case OP_NOP6: case OP_NOP7: case OP_NOP8: case OP_NOP9: case OP_NOP10:
                {
                    if (flags & SCRIPT_VERIFY_DISCOURAGE_UPGRADABLE_NOPS)
                        return set_error(serror, SCRIPT_ERR_DISCOURAGE_UPGRADABLE_NOPS);
                }
                break;
                case OP_IF:
                case OP_NOTIF:
                {
                    bool fValue = false;
                    if (fExec)
                    {
                        if (stack.size() < 1)
                            return set_error(serror, SCRIPT_ERR_UNBALANCED_CONDITIONAL);
                        valtype& vch = stacktop(-1);
                        if (sigversion == SigVersion::TAPSCRIPT) {
                            if (vch.size() > 1 || (vch.size() == 1 && vch[0] != 1)) {
                                return set_error(serror, SCRIPT_ERR_TAPSCRIPT_MINIMALIF);
                            }
                        }
                        if (sigversion == SigVersion::WITNESS_V0 && (flags & SCRIPT_VERIFY_MINIMALIF)) {
                            if (vch.size() > 1)
                                return set_error(serror, SCRIPT_ERR_MINIMALIF);
                            if (vch.size() == 1 && vch[0] != 1)
                                return set_error(serror, SCRIPT_ERR_MINIMALIF);
                        }
                        fValue = CastToBool(vch);
                        if (opcode == OP_NOTIF)
                            fValue = !fValue;
                        popstack(stack);
                    }
                    vfExec.push_back(fValue);
                }
                break;
                case OP_ELSE:
                {
                    if (vfExec.empty())
                        return set_error(serror, SCRIPT_ERR_UNBALANCED_CONDITIONAL);
                    vfExec.toggle_top();
                }
                break;
                case OP_ENDIF:
                {
                    if (vfExec.empty())
                        return set_error(serror, SCRIPT_ERR_UNBALANCED_CONDITIONAL);
                    vfExec.pop_back();
                }
                break;
                case OP_VERIFY:
                {
                    if (stack.size() < 1)
                        return set_error(serror, SCRIPT_ERR_INVALID_STACK_OPERATION);
                    bool fValue = CastToBool(stacktop(-1));
                    if (fValue)
                        popstack(stack);
                    else
                        return set_error(serror, SCRIPT_ERR_VERIFY);
                }
                break;
                case OP_RETURN:
                {
                    return set_error(serror, SCRIPT_ERR_OP_RETURN);
                }
                break;
                case OP_TOALTSTACK:
                {
                    if (stack.size() < 1)
                        return set_error(serror, SCRIPT_ERR_INVALID_STACK_OPERATION);
                    altstack.push_back(stacktop(-1));
                    popstack(stack);
                }
                break;
                case OP_FROMALTSTACK:
                {
                    if (altstack.size() < 1)
                        return set_error(serror, SCRIPT_ERR_INVALID_ALTSTACK_OPERATION);
                    stack.push_back(altstacktop(-1));
                    popstack(altstack);
                }
                break;
                case OP_2DROP:
                {
                    if (stack.size() < 2)
                        return set_error(serror, SCRIPT_ERR_INVALID_STACK_OPERATION);
                    popstack(stack);
                    popstack(stack);
                }
                break;
                case OP_2DUP:
                {
                    if (stack.size() < 2)
                        return set_error(serror, SCRIPT_ERR_INVALID_STACK_OPERATION);
                    valtype vch1 = stacktop(-2);
                    valtype vch2 = stacktop(-1);
                    stack.push_back(vch1);
                    stack.push_back(vch2);
                }
                break;
                case OP_3DUP:
                {
                    if (stack.size() < 3)
                        return set_error(serror, SCRIPT_ERR_INVALID_STACK_OPERATION);
                    valtype vch1 = stacktop(-3);
                    valtype vch2 = stacktop(-2);
                    valtype vch3 = stacktop(-1);
                    stack.push_back(vch1);
                    stack.push_back(vch2);
                    stack.push_back(vch3);
                }
                break;
                case OP_2OVER:
                {
                    if (stack.size() < 4)
                        return set_error(serror, SCRIPT_ERR_INVALID_STACK_OPERATION);
                    valtype vch1 = stacktop(-4);
                    valtype vch2 = stacktop(-3);
                    stack.push_back(vch1);
                    stack.push_back(vch2);
                }
                break;
                case OP_2ROT:
                {
                    if (stack.size() < 6)
                        return set_error(serror, SCRIPT_ERR_INVALID_STACK_OPERATION);
                    valtype vch1 = stacktop(-6);
                    valtype vch2 = stacktop(-5);
                    stack.erase(stack.end()-6, stack.end()-4);
                    stack.push_back(vch1);
                    stack.push_back(vch2);
                }
                break;
                case OP_2SWAP:
                {
                    if (stack.size() < 4)
                        return set_error(serror, SCRIPT_ERR_INVALID_STACK_OPERATION);
                    swap(stacktop(-4), stacktop(-2));
                    swap(stacktop(-3), stacktop(-1));
                }
                break;
                case OP_IFDUP:
                {
                    if (stack.size() < 1)
                        return set_error(serror, SCRIPT_ERR_INVALID_STACK_OPERATION);
                    valtype vch = stacktop(-1);
                    if (CastToBool(vch))
                        stack.push_back(vch);
                }
                break;
                case OP_DEPTH:
                {
                    CScriptNum bn(stack.size());
                    stack.push_back(bn.getvch());
                }
                break;
                case OP_DROP:
                {
                    if (stack.size() < 1)
                        return set_error(serror, SCRIPT_ERR_INVALID_STACK_OPERATION);
                    popstack(stack);
                }
                break;
                case OP_DUP:
                {
                    if (stack.size() < 1)
                        return set_error(serror, SCRIPT_ERR_INVALID_STACK_OPERATION);
                    valtype vch = stacktop(-1);
                    stack.push_back(vch);
                }
                break;
                case OP_NIP:
                {
                    if (stack.size() < 2)
                        return set_error(serror, SCRIPT_ERR_INVALID_STACK_OPERATION);
                    stack.erase(stack.end() - 2);
                }
                break;
                case OP_OVER:
                {
                    if (stack.size() < 2)
                        return set_error(serror, SCRIPT_ERR_INVALID_STACK_OPERATION);
                    valtype vch = stacktop(-2);
                    stack.push_back(vch);
                }
                break;
                case OP_PICK:
                case OP_ROLL:
                {
                    if (stack.size() < 2)
                        return set_error(serror, SCRIPT_ERR_INVALID_STACK_OPERATION);
                    int n = CScriptNum(stacktop(-1), fRequireMinimal).getint();
                    popstack(stack);
                    if (n < 0 || n >= (int)stack.size())
                        return set_error(serror, SCRIPT_ERR_INVALID_STACK_OPERATION);
                    valtype vch = stacktop(-n-1);
                    if (opcode == OP_ROLL)
                        stack.erase(stack.end()-n-1);
                    stack.push_back(vch);
                }
                break;
                case OP_ROT:
                {
                    if (stack.size() < 3)
                        return set_error(serror, SCRIPT_ERR_INVALID_STACK_OPERATION);
                    swap(stacktop(-3), stacktop(-2));
                    swap(stacktop(-2), stacktop(-1));
                }
                break;
                case OP_SWAP:
                {
                    if (stack.size() < 2)
                        return set_error(serror, SCRIPT_ERR_INVALID_STACK_OPERATION);
                    swap(stacktop(-2), stacktop(-1));
                }
                break;
                case OP_TUCK:
                {
                    if (stack.size() < 2)
                        return set_error(serror, SCRIPT_ERR_INVALID_STACK_OPERATION);
                    valtype vch = stacktop(-1);
                    stack.insert(stack.end()-2, vch);
                }
                break;
                case OP_SIZE:
                {
                    if (stack.size() < 1)
                        return set_error(serror, SCRIPT_ERR_INVALID_STACK_OPERATION);
                    CScriptNum bn(stacktop(-1).size());
                    stack.push_back(bn.getvch());
                }
                break;
                case OP_EQUAL:
                case OP_EQUALVERIFY:
                {
                    if (stack.size() < 2)
                        return set_error(serror, SCRIPT_ERR_INVALID_STACK_OPERATION);
                    valtype& vch1 = stacktop(-2);
                    valtype& vch2 = stacktop(-1);
                    bool fEqual = (vch1 == vch2);
                    popstack(stack);
                    popstack(stack);
                    stack.push_back(fEqual ? vchTrue : vchFalse);
                    if (opcode == OP_EQUALVERIFY)
                    {
                        if (fEqual)
                            popstack(stack);
                        else
                            return set_error(serror, SCRIPT_ERR_EQUALVERIFY);
                    }
                }
                break;
                case OP_1ADD:
                case OP_1SUB:
                case OP_NEGATE:
                case OP_ABS:
                case OP_NOT:
                case OP_0NOTEQUAL:
                {
                    if (stack.size() < 1)
                        return set_error(serror, SCRIPT_ERR_INVALID_STACK_OPERATION);
                    CScriptNum bn(stacktop(-1), fRequireMinimal);
                    switch (opcode)
                    {
                    case OP_1ADD:       bn += bnOne; break;
                    case OP_1SUB:       bn -= bnOne; break;
                    case OP_NEGATE:     bn = -bn; break;
                    case OP_ABS:        if (bn < bnZero) bn = -bn; break;
                    case OP_NOT:        bn = (bn == bnZero); break;
                    case OP_0NOTEQUAL:  bn = (bn != bnZero); break;
                    default:            assert(!"invalid opcode"); break;
                    }
                    popstack(stack);
                    stack.push_back(bn.getvch());
                }
                break;
                case OP_ADD:
                case OP_SUB:
                case OP_BOOLAND:
                case OP_BOOLOR:
                case OP_NUMEQUAL:
                case OP_NUMEQUALVERIFY:
                case OP_NUMNOTEQUAL:
                case OP_LESSTHAN:
                case OP_GREATERTHAN:
                case OP_LESSTHANOREQUAL:
                case OP_GREATERTHANOREQUAL:
                case OP_MIN:
                case OP_MAX:
                {
                    if (stack.size() < 2)
                        return set_error(serror, SCRIPT_ERR_INVALID_STACK_OPERATION);
                    CScriptNum bn1(stacktop(-2), fRequireMinimal);
                    CScriptNum bn2(stacktop(-1), fRequireMinimal);
                    CScriptNum bn(0);
                    switch (opcode)
                    {
                    case OP_ADD:
                        bn = bn1 + bn2;
                        break;
                    case OP_SUB:
                        bn = bn1 - bn2;
                        break;
                    case OP_BOOLAND:             bn = (bn1 != bnZero && bn2 != bnZero); break;
                    case OP_BOOLOR:              bn = (bn1 != bnZero || bn2 != bnZero); break;
                    case OP_NUMEQUAL:            bn = (bn1 == bn2); break;
                    case OP_NUMEQUALVERIFY:      bn = (bn1 == bn2); break;
                    case OP_NUMNOTEQUAL:         bn = (bn1 != bn2); break;
                    case OP_LESSTHAN:            bn = (bn1 < bn2); break;
                    case OP_GREATERTHAN:         bn = (bn1 > bn2); break;
                    case OP_LESSTHANOREQUAL:     bn = (bn1 <= bn2); break;
                    case OP_GREATERTHANOREQUAL:  bn = (bn1 >= bn2); break;
                    case OP_MIN:                 bn = (bn1 < bn2 ? bn1 : bn2); break;
                    case OP_MAX:                 bn = (bn1 > bn2 ? bn1 : bn2); break;
                    default:                     assert(!"invalid opcode"); break;
                    }
                    popstack(stack);
                    popstack(stack);
                    stack.push_back(bn.getvch());
                    if (opcode == OP_NUMEQUALVERIFY)
                    {
                        if (CastToBool(stacktop(-1)))
                            popstack(stack);
                        else
                            return set_error(serror, SCRIPT_ERR_NUMEQUALVERIFY);
                    }
                }
                break;
                case OP_WITHIN:
                {
                    if (stack.size() < 3)
                        return set_error(serror, SCRIPT_ERR_INVALID_STACK_OPERATION);
                    CScriptNum bn1(stacktop(-3), fRequireMinimal);
                    CScriptNum bn2(stacktop(-2), fRequireMinimal);
                    CScriptNum bn3(stacktop(-1), fRequireMinimal);
                    bool fValue = (bn2 <= bn1 && bn1 < bn3);
                    popstack(stack);
                    popstack(stack);
                    popstack(stack);
                    stack.push_back(fValue ? vchTrue : vchFalse);
                }
                break;
                case OP_RIPEMD160:
                case OP_SHA1:
                case OP_SHA256:
                case OP_HASH160:
                case OP_HASH256:
                {
                    if (stack.size() < 1)
                        return set_error(serror, SCRIPT_ERR_INVALID_STACK_OPERATION);
                    valtype& vch = stacktop(-1);
                    valtype vchHash((opcode == OP_RIPEMD160 || opcode == OP_SHA1 || opcode == OP_HASH160) ? 20 : 32);
                    if (opcode == OP_RIPEMD160)
                        CRIPEMD160().Write(vch.data(), vch.size()).Finalize(vchHash.data());
                    else if (opcode == OP_SHA1)
                        CSHA1().Write(vch.data(), vch.size()).Finalize(vchHash.data());
                    else if (opcode == OP_SHA256)
                        CSHA256().Write(vch.data(), vch.size()).Finalize(vchHash.data());
                    else if (opcode == OP_HASH160)
                        CHash160().Write(vch).Finalize(vchHash);
                    else if (opcode == OP_HASH256)
                        CHash256().Write(vch).Finalize(vchHash);
                    popstack(stack);
                    stack.push_back(vchHash);
                }
                break;
                case OP_CODESEPARATOR:
                {
                    pbegincodehash = pc;
                    execdata.m_codeseparator_pos = opcode_pos;
                }
                break;
                case OP_CHECKSIG:
                case OP_CHECKSIGVERIFY:
                {
                    if (stack.size() < 2)
                        return set_error(serror, SCRIPT_ERR_INVALID_STACK_OPERATION);
                    valtype& vchSig    = stacktop(-2);
                    valtype& vchPubKey = stacktop(-1);
                    bool fSuccess = true;
                    if (!EvalChecksig(vchSig, vchPubKey, pbegincodehash, pend, execdata, flags, checker, sigversion, serror, fSuccess)) return false;
                    popstack(stack);
                    popstack(stack);
                    stack.push_back(fSuccess ? vchTrue : vchFalse);
                    if (opcode == OP_CHECKSIGVERIFY)
                    {
                        if (fSuccess)
                            popstack(stack);
                        else
                            return set_error(serror, SCRIPT_ERR_CHECKSIGVERIFY);
                    }
                }
                break;
                case OP_CHECKSIGADD:
                {
                    if (sigversion == SigVersion::BASE || sigversion == SigVersion::WITNESS_V0) return set_error(serror, SCRIPT_ERR_BAD_OPCODE);
                    if (stack.size() < 3) return set_error(serror, SCRIPT_ERR_INVALID_STACK_OPERATION);
                    const valtype& sig = stacktop(-3);
                    const CScriptNum num(stacktop(-2), fRequireMinimal);
                    const valtype& pubkey = stacktop(-1);
                    bool success = true;
                    if (!EvalChecksig(sig, pubkey, pbegincodehash, pend, execdata, flags, checker, sigversion, serror, success)) return false;
                    popstack(stack);
                    popstack(stack);
                    popstack(stack);
                    stack.push_back((num + (success ? 1 : 0)).getvch());
                }
                break;
                case OP_CHECKMULTISIG:
                case OP_CHECKMULTISIGVERIFY:
                {
                    if (sigversion == SigVersion::TAPSCRIPT) return set_error(serror, SCRIPT_ERR_TAPSCRIPT_CHECKMULTISIG);
                    int i = 1;
                    if ((int)stack.size() < i)
                        return set_error(serror, SCRIPT_ERR_INVALID_STACK_OPERATION);
                    int nKeysCount = CScriptNum(stacktop(-i), fRequireMinimal).getint();
                    if (nKeysCount < 0 || nKeysCount > MAX_PUBKEYS_PER_MULTISIG)
                        return set_error(serror, SCRIPT_ERR_PUBKEY_COUNT);
                    nOpCount += nKeysCount;
                    if (nOpCount > MAX_OPS_PER_SCRIPT)
                        return set_error(serror, SCRIPT_ERR_OP_COUNT);
                    int ikey = ++i;
                    int ikey2 = nKeysCount + 2;
                    i += nKeysCount;
                    if ((int)stack.size() < i)
                        return set_error(serror, SCRIPT_ERR_INVALID_STACK_OPERATION);
                    int nSigsCount = CScriptNum(stacktop(-i), fRequireMinimal).getint();
                    if (nSigsCount < 0 || nSigsCount > nKeysCount)
                        return set_error(serror, SCRIPT_ERR_SIG_COUNT);
                    int isig = ++i;
                    i += nSigsCount;
                    if ((int)stack.size() < i)
                        return set_error(serror, SCRIPT_ERR_INVALID_STACK_OPERATION);
                    CScript scriptCode(pbegincodehash, pend);
                    for (int k = 0; k < nSigsCount; k++)
                    {
                        valtype& vchSig = stacktop(-isig-k);
                        if (sigversion == SigVersion::BASE) {
                            int found = FindAndDelete(scriptCode, CScript() << vchSig);
                            if (found > 0 && (flags & SCRIPT_VERIFY_CONST_SCRIPTCODE))
                                return set_error(serror, SCRIPT_ERR_SIG_FINDANDDELETE);
                        }
                    }
                    bool fSuccess = true;
                    while (fSuccess && nSigsCount > 0)
                    {
                        valtype& vchSig    = stacktop(-isig);
                        valtype& vchPubKey = stacktop(-ikey);
                        if (!CheckSignatureEncoding(vchSig, flags, serror) || !CheckPubKeyEncoding(vchPubKey, flags, sigversion, serror)) {
                            return false;
                        }
                        bool fOk = checker.CheckECDSASignature(vchSig, vchPubKey, scriptCode, sigversion);
                        if (fOk) {
                            isig++;
                            nSigsCount--;
                        }
                        ikey++;
                        nKeysCount--;
                        if (nSigsCount > nKeysCount)
                            fSuccess = false;
                    }
                    while (i-- > 1) {
                        if (!fSuccess && (flags & SCRIPT_VERIFY_NULLFAIL) && !ikey2 && stacktop(-1).size())
                            return set_error(serror, SCRIPT_ERR_SIG_NULLFAIL);
                        if (ikey2 > 0)
                            ikey2--;
                        popstack(stack);
                    }
                    if (stack.size() < 1)
                        return set_error(serror, SCRIPT_ERR_INVALID_STACK_OPERATION);
                    if ((flags & SCRIPT_VERIFY_NULLDUMMY) && stacktop(-1).size())
                        return set_error(serror, SCRIPT_ERR_SIG_NULLDUMMY);
                    popstack(stack);
                    stack.push_back(fSuccess ? vchTrue : vchFalse);
                    if (opcode == OP_CHECKMULTISIGVERIFY)
                    {
                        if (fSuccess)
                            popstack(stack);
                        else
                            return set_error(serror, SCRIPT_ERR_CHECKMULTISIGVERIFY);
                    }
                }
                break;
                default:
                    return set_error(serror, SCRIPT_ERR_BAD_OPCODE);
            }
            if (stack.size() + altstack.size() > MAX_STACK_SIZE)
                return set_error(serror, SCRIPT_ERR_STACK_SIZE);
        }
    }
    catch (...)
    {
        return set_error(serror, SCRIPT_ERR_UNKNOWN_ERROR);
    }
    if (!vfExec.empty())
        return set_error(serror, SCRIPT_ERR_UNBALANCED_CONDITIONAL);
    return set_success(serror);
}
bool EvalScript(std::vector<std::vector<unsigned char> >& stack, const CScript& script, unsigned int flags, const BaseSignatureChecker& checker, SigVersion sigversion, ScriptError* serror)
{
    ScriptExecutionData execdata;
    return EvalScript(stack, script, flags, checker, sigversion, execdata, serror);
}
namespace {
template <class T>
class CTransactionSignatureSerializer
{
private:
    const T& txTo;
    const CScript& scriptCode;
    const unsigned int nIn;
    const bool fAnyoneCanPay;
    const bool fHashSingle;
    const bool fHashNone;
public:
    CTransactionSignatureSerializer(const T& txToIn, const CScript& scriptCodeIn, unsigned int nInIn, int nHashTypeIn) :
        txTo(txToIn), scriptCode(scriptCodeIn), nIn(nInIn),
        fAnyoneCanPay(!!(nHashTypeIn & SIGHASH_ANYONECANPAY)),
        fHashSingle((nHashTypeIn & 0x1f) == SIGHASH_SINGLE),
        fHashNone((nHashTypeIn & 0x1f) == SIGHASH_NONE) {}
    template<typename S>
    void SerializeScriptCode(S &s) const {
        CScript::const_iterator it = scriptCode.begin();
        CScript::const_iterator itBegin = it;
        opcodetype opcode;
        unsigned int nCodeSeparators = 0;
        while (scriptCode.GetOp(it, opcode)) {
            if (opcode == OP_CODESEPARATOR)
                nCodeSeparators++;
        }
        ::WriteCompactSize(s, scriptCode.size() - nCodeSeparators);
        it = itBegin;
        while (scriptCode.GetOp(it, opcode)) {
            if (opcode == OP_CODESEPARATOR) {
                s.write(AsBytes(Span{&itBegin[0], size_t(it - itBegin - 1)}));
                itBegin = it;
            }
        }
        if (itBegin != scriptCode.end())
            s.write(AsBytes(Span{&itBegin[0], size_t(it - itBegin)}));
    }
    template<typename S>
    void SerializeInput(S &s, unsigned int nInput) const {
        if (fAnyoneCanPay)
            nInput = nIn;
        ::Serialize(s, txTo.vin[nInput].prevout);
        if (nInput != nIn)
            ::Serialize(s, CScript());
        else
            SerializeScriptCode(s);
        if (nInput != nIn && (fHashSingle || fHashNone))
            ::Serialize(s, (int)0);
        else
            ::Serialize(s, txTo.vin[nInput].nSequence);
    }
    template<typename S>
    void SerializeOutput(S &s, unsigned int nOutput) const {
        if (fHashSingle && nOutput != nIn)
            ::Serialize(s, CTxOut());
        else
            ::Serialize(s, txTo.vout[nOutput]);
    }
    template<typename S>
    void Serialize(S &s) const {
        ::Serialize(s, txTo.nVersion);
        unsigned int nInputs = fAnyoneCanPay ? 1 : txTo.vin.size();
        ::WriteCompactSize(s, nInputs);
        for (unsigned int nInput = 0; nInput < nInputs; nInput++)
             SerializeInput(s, nInput);
        unsigned int nOutputs = fHashNone ? 0 : (fHashSingle ? nIn+1 : txTo.vout.size());
        ::WriteCompactSize(s, nOutputs);
        for (unsigned int nOutput = 0; nOutput < nOutputs; nOutput++)
             SerializeOutput(s, nOutput);
        ::Serialize(s, txTo.nLockTime);
    }
};
template <class T>
uint256 GetPrevoutsSHA256(const T& txTo)
{
    HashWriter ss{};
    for (const auto& txin : txTo.vin) {
        ss << txin.prevout;
    }
    return ss.GetSHA256();
}
template <class T>
uint256 GetSequencesSHA256(const T& txTo)
{
    HashWriter ss{};
    for (const auto& txin : txTo.vin) {
        ss << txin.nSequence;
    }
    return ss.GetSHA256();
}
template <class T>
uint256 GetOutputsSHA256(const T& txTo)
{
    HashWriter ss{};
    for (const auto& txout : txTo.vout) {
        ss << txout;
    }
    return ss.GetSHA256();
}
uint256 GetSpentAmountsSHA256(const std::vector<CTxOut>& outputs_spent)
{
    HashWriter ss{};
    for (const auto& txout : outputs_spent) {
        ss << txout.nValue;
    }
    return ss.GetSHA256();
}
uint256 GetSpentScriptsSHA256(const std::vector<CTxOut>& outputs_spent)
{
    HashWriter ss{};
    for (const auto& txout : outputs_spent) {
        ss << txout.scriptPubKey;
    }
    return ss.GetSHA256();
}
}
template <class T>
void PrecomputedTransactionData::Init(const T& txTo, std::vector<CTxOut>&& spent_outputs, bool force)
{
    assert(!m_spent_outputs_ready);
    m_spent_outputs = std::move(spent_outputs);
    if (!m_spent_outputs.empty()) {
        assert(m_spent_outputs.size() == txTo.vin.size());
        m_spent_outputs_ready = true;
    }
    bool uses_bip143_segwit = force;
    bool uses_bip341_taproot = force;
    for (size_t inpos = 0; inpos < txTo.vin.size() && !(uses_bip143_segwit && uses_bip341_taproot); ++inpos) {
        if (!txTo.vin[inpos].scriptWitness.IsNull()) {
            if (m_spent_outputs_ready && m_spent_outputs[inpos].scriptPubKey.size() == 2 + WITNESS_V1_TAPROOT_SIZE &&
                m_spent_outputs[inpos].scriptPubKey[0] == OP_1) {
                uses_bip341_taproot = true;
            } else {
                uses_bip143_segwit = true;
            }
        }
        if (uses_bip341_taproot && uses_bip143_segwit) break;
    }
    if (uses_bip143_segwit || uses_bip341_taproot) {
        m_prevouts_single_hash = GetPrevoutsSHA256(txTo);
        m_sequences_single_hash = GetSequencesSHA256(txTo);
        m_outputs_single_hash = GetOutputsSHA256(txTo);
    }
    if (uses_bip143_segwit) {
        hashPrevouts = SHA256Uint256(m_prevouts_single_hash);
        hashSequence = SHA256Uint256(m_sequences_single_hash);
        hashOutputs = SHA256Uint256(m_outputs_single_hash);
        m_bip143_segwit_ready = true;
    }
    if (uses_bip341_taproot) {
        m_spent_amounts_single_hash = GetSpentAmountsSHA256(m_spent_outputs);
        m_spent_scripts_single_hash = GetSpentScriptsSHA256(m_spent_outputs);
        m_bip341_taproot_ready = true;
    }
}
template <class T>
PrecomputedTransactionData::PrecomputedTransactionData(const T& txTo)
{
    Init(txTo, {});
}
template void PrecomputedTransactionData::Init(const CTransaction& txTo, std::vector<CTxOut>&& spent_outputs, bool force);
template void PrecomputedTransactionData::Init(const CMutableTransaction& txTo, std::vector<CTxOut>&& spent_outputs, bool force);
template PrecomputedTransactionData::PrecomputedTransactionData(const CTransaction& txTo);
template PrecomputedTransactionData::PrecomputedTransactionData(const CMutableTransaction& txTo);
const HashWriter HASHER_TAPSIGHASH{TaggedHash("TapSighash")};
const HashWriter HASHER_TAPLEAF{TaggedHash("TapLeaf")};
const HashWriter HASHER_TAPBRANCH{TaggedHash("TapBranch")};
static bool HandleMissingData(MissingDataBehavior mdb)
{
    switch (mdb) {
    case MissingDataBehavior::ASSERT_FAIL:
        assert(!"Missing data");
        break;
    case MissingDataBehavior::FAIL:
        return false;
    }
    assert(!"Unknown MissingDataBehavior value");
}
template<typename T>
bool SignatureHashSchnorr(uint256& hash_out, ScriptExecutionData& execdata, const T& tx_to, uint32_t in_pos, uint8_t hash_type, SigVersion sigversion, const PrecomputedTransactionData& cache, MissingDataBehavior mdb)
{
    uint8_t ext_flag, key_version;
    switch (sigversion) {
    case SigVersion::TAPROOT:
        ext_flag = 0;
        break;
    case SigVersion::TAPSCRIPT:
        ext_flag = 1;
        key_version = 0;
        break;
    default:
        assert(false);
    }
    assert(in_pos < tx_to.vin.size());
    if (!(cache.m_bip341_taproot_ready && cache.m_spent_outputs_ready)) {
        return HandleMissingData(mdb);
    }
    HashWriter ss{HASHER_TAPSIGHASH};
    static constexpr uint8_t EPOCH = 0;
    ss << EPOCH;
    const uint8_t output_type = (hash_type == SIGHASH_DEFAULT) ? SIGHASH_ALL : (hash_type & SIGHASH_OUTPUT_MASK);
    const uint8_t input_type = hash_type & SIGHASH_INPUT_MASK;
    if (!(hash_type <= 0x03 || (hash_type >= 0x81 && hash_type <= 0x83))) return false;
    ss << hash_type;
    ss << tx_to.nVersion;
    ss << tx_to.nLockTime;
    if (input_type != SIGHASH_ANYONECANPAY) {
        ss << cache.m_prevouts_single_hash;
        ss << cache.m_spent_amounts_single_hash;
        ss << cache.m_spent_scripts_single_hash;
        ss << cache.m_sequences_single_hash;
    }
    if (output_type == SIGHASH_ALL) {
        ss << cache.m_outputs_single_hash;
    }
    assert(execdata.m_annex_init);
    const bool have_annex = execdata.m_annex_present;
    const uint8_t spend_type = (ext_flag << 1) + (have_annex ? 1 : 0);
    ss << spend_type;
    if (input_type == SIGHASH_ANYONECANPAY) {
        ss << tx_to.vin[in_pos].prevout;
        ss << cache.m_spent_outputs[in_pos];
        ss << tx_to.vin[in_pos].nSequence;
    } else {
        ss << in_pos;
    }
    if (have_annex) {
        ss << execdata.m_annex_hash;
    }
    if (output_type == SIGHASH_SINGLE) {
        if (in_pos >= tx_to.vout.size()) return false;
        if (!execdata.m_output_hash) {
            HashWriter sha_single_output{};
            sha_single_output << tx_to.vout[in_pos];
            execdata.m_output_hash = sha_single_output.GetSHA256();
        }
        ss << execdata.m_output_hash.value();
    }
    if (sigversion == SigVersion::TAPSCRIPT) {
        assert(execdata.m_tapleaf_hash_init);
        ss << execdata.m_tapleaf_hash;
        ss << key_version;
        assert(execdata.m_codeseparator_pos_init);
        ss << execdata.m_codeseparator_pos;
    }
    hash_out = ss.GetSHA256();
    return true;
}
template <class T>
uint256 SignatureHash(const CScript& scriptCode, const T& txTo, unsigned int nIn, int nHashType, const CAmount& amount, SigVersion sigversion, const PrecomputedTransactionData* cache)
{
    assert(nIn < txTo.vin.size());
    if (sigversion == SigVersion::WITNESS_V0) {
        uint256 hashPrevouts;
        uint256 hashSequence;
        uint256 hashOutputs;
        const bool cacheready = cache && cache->m_bip143_segwit_ready;
        if (!(nHashType & SIGHASH_ANYONECANPAY)) {
            hashPrevouts = cacheready ? cache->hashPrevouts : SHA256Uint256(GetPrevoutsSHA256(txTo));
        }
        if (!(nHashType & SIGHASH_ANYONECANPAY) && (nHashType & 0x1f) != SIGHASH_SINGLE && (nHashType & 0x1f) != SIGHASH_NONE) {
            hashSequence = cacheready ? cache->hashSequence : SHA256Uint256(GetSequencesSHA256(txTo));
        }
        if ((nHashType & 0x1f) != SIGHASH_SINGLE && (nHashType & 0x1f) != SIGHASH_NONE) {
            hashOutputs = cacheready ? cache->hashOutputs : SHA256Uint256(GetOutputsSHA256(txTo));
        } else if ((nHashType & 0x1f) == SIGHASH_SINGLE && nIn < txTo.vout.size()) {
            HashWriter ss{};
            ss << txTo.vout[nIn];
            hashOutputs = ss.GetHash();
        }
        HashWriter ss{};
        ss << txTo.nVersion;
        ss << hashPrevouts;
        ss << hashSequence;
        ss << txTo.vin[nIn].prevout;
        ss << scriptCode;
        ss << amount;
        ss << txTo.vin[nIn].nSequence;
        ss << hashOutputs;
        ss << txTo.nLockTime;
        ss << nHashType;
        return ss.GetHash();
    }
    if ((nHashType & 0x1f) == SIGHASH_SINGLE) {
        if (nIn >= txTo.vout.size()) {
            return uint256::ONE;
        }
    }
    CTransactionSignatureSerializer<T> txTmp(txTo, scriptCode, nIn, nHashType);
    HashWriter ss{};
    ss << txTmp << nHashType;
    return ss.GetHash();
}
template <class T>
bool GenericTransactionSignatureChecker<T>::VerifyECDSASignature(const std::vector<unsigned char>& vchSig, const CPubKey& pubkey, const uint256& sighash) const
{
    return pubkey.Verify(sighash, vchSig);
}
template <class T>
bool GenericTransactionSignatureChecker<T>::VerifySchnorrSignature(Span<const unsigned char> sig, const XOnlyPubKey& pubkey, const uint256& sighash) const
{
    return pubkey.VerifySchnorr(sighash, sig);
}
template <class T>
bool GenericTransactionSignatureChecker<T>::CheckECDSASignature(const std::vector<unsigned char>& vchSigIn, const std::vector<unsigned char>& vchPubKey, const CScript& scriptCode, SigVersion sigversion) const
{
    CPubKey pubkey(vchPubKey);
    if (!pubkey.IsValid())
        return false;
    std::vector<unsigned char> vchSig(vchSigIn);
    if (vchSig.empty())
        return false;
    int nHashType = vchSig.back();
    vchSig.pop_back();
    if (sigversion == SigVersion::WITNESS_V0 && amount < 0) return HandleMissingData(m_mdb);
    uint256 sighash = SignatureHash(scriptCode, *txTo, nIn, nHashType, amount, sigversion, this->txdata);
    if (!VerifyECDSASignature(vchSig, pubkey, sighash))
        return false;
    return true;
}
template <class T>
bool GenericTransactionSignatureChecker<T>::CheckSchnorrSignature(Span<const unsigned char> sig, Span<const unsigned char> pubkey_in, SigVersion sigversion, ScriptExecutionData& execdata, ScriptError* serror) const
{
    assert(sigversion == SigVersion::TAPROOT || sigversion == SigVersion::TAPSCRIPT);
    assert(pubkey_in.size() == 32);
    if (sig.size() != 64 && sig.size() != 65) return set_error(serror, SCRIPT_ERR_SCHNORR_SIG_SIZE);
    XOnlyPubKey pubkey{pubkey_in};
    uint8_t hashtype = SIGHASH_DEFAULT;
    if (sig.size() == 65) {
        hashtype = SpanPopBack(sig);
        if (hashtype == SIGHASH_DEFAULT) return set_error(serror, SCRIPT_ERR_SCHNORR_SIG_HASHTYPE);
    }
    uint256 sighash;
    if (!this->txdata) return HandleMissingData(m_mdb);
    if (!SignatureHashSchnorr(sighash, execdata, *txTo, nIn, hashtype, sigversion, *this->txdata, m_mdb)) {
        return set_error(serror, SCRIPT_ERR_SCHNORR_SIG_HASHTYPE);
    }
    if (!VerifySchnorrSignature(sig, pubkey, sighash)) return set_error(serror, SCRIPT_ERR_SCHNORR_SIG);
    return true;
}
template <class T>
bool GenericTransactionSignatureChecker<T>::CheckLockTime(const CScriptNum& nLockTime) const
{
    if (!(
        (txTo->nLockTime <  LOCKTIME_THRESHOLD && nLockTime <  LOCKTIME_THRESHOLD) ||
        (txTo->nLockTime >= LOCKTIME_THRESHOLD && nLockTime >= LOCKTIME_THRESHOLD)
    ))
        return false;
    if (nLockTime > (int64_t)txTo->nLockTime)
        return false;
    if (CTxIn::SEQUENCE_FINAL == txTo->vin[nIn].nSequence)
        return false;
    return true;
}
template <class T>
bool GenericTransactionSignatureChecker<T>::CheckSequence(const CScriptNum& nSequence) const
{
    const int64_t txToSequence = (int64_t)txTo->vin[nIn].nSequence;
    if (static_cast<uint32_t>(txTo->nVersion) < 2)
        return false;
    if (txToSequence & CTxIn::SEQUENCE_LOCKTIME_DISABLE_FLAG)
        return false;
    const uint32_t nLockTimeMask = CTxIn::SEQUENCE_LOCKTIME_TYPE_FLAG | CTxIn::SEQUENCE_LOCKTIME_MASK;
    const int64_t txToSequenceMasked = txToSequence & nLockTimeMask;
    const CScriptNum nSequenceMasked = nSequence & nLockTimeMask;
    if (!(
        (txToSequenceMasked <  CTxIn::SEQUENCE_LOCKTIME_TYPE_FLAG && nSequenceMasked <  CTxIn::SEQUENCE_LOCKTIME_TYPE_FLAG) ||
        (txToSequenceMasked >= CTxIn::SEQUENCE_LOCKTIME_TYPE_FLAG && nSequenceMasked >= CTxIn::SEQUENCE_LOCKTIME_TYPE_FLAG)
    )) {
        return false;
    }
    if (nSequenceMasked > txToSequenceMasked)
        return false;
    return true;
}
template class GenericTransactionSignatureChecker<CTransaction>;
template class GenericTransactionSignatureChecker<CMutableTransaction>;
static bool ExecuteWitnessScript(const Span<const valtype>& stack_span, const CScript& exec_script, unsigned int flags, SigVersion sigversion, const BaseSignatureChecker& checker, ScriptExecutionData& execdata, ScriptError* serror)
{
    std::vector<valtype> stack{stack_span.begin(), stack_span.end()};
    if (sigversion == SigVersion::TAPSCRIPT) {
        CScript::const_iterator pc = exec_script.begin();
        while (pc < exec_script.end()) {
            opcodetype opcode;
            if (!exec_script.GetOp(pc, opcode)) {
                return set_error(serror, SCRIPT_ERR_BAD_OPCODE);
            }
            if (IsOpSuccess(opcode)) {
                if (flags & SCRIPT_VERIFY_DISCOURAGE_OP_SUCCESS) {
                    return set_error(serror, SCRIPT_ERR_DISCOURAGE_OP_SUCCESS);
                }
                return set_success(serror);
            }
        }
        if (stack.size() > MAX_STACK_SIZE) return set_error(serror, SCRIPT_ERR_STACK_SIZE);
    }
    for (const valtype& elem : stack) {
        if (elem.size() > MAX_SCRIPT_ELEMENT_SIZE) return set_error(serror, SCRIPT_ERR_PUSH_SIZE);
    }
    if (!EvalScript(stack, exec_script, flags, checker, sigversion, execdata, serror)) return false;
    if (stack.size() != 1) return set_error(serror, SCRIPT_ERR_CLEANSTACK);
    if (!CastToBool(stack.back())) return set_error(serror, SCRIPT_ERR_EVAL_FALSE);
    return true;
}
uint256 ComputeTapleafHash(uint8_t leaf_version, const CScript& script)
{
    return (HashWriter{HASHER_TAPLEAF} << leaf_version << script).GetSHA256();
}
uint256 ComputeTaprootMerkleRoot(Span<const unsigned char> control, const uint256& tapleaf_hash)
{
    assert(control.size() >= TAPROOT_CONTROL_BASE_SIZE);
    assert(control.size() <= TAPROOT_CONTROL_MAX_SIZE);
    assert((control.size() - TAPROOT_CONTROL_BASE_SIZE) % TAPROOT_CONTROL_NODE_SIZE == 0);
    const int path_len = (control.size() - TAPROOT_CONTROL_BASE_SIZE) / TAPROOT_CONTROL_NODE_SIZE;
    uint256 k = tapleaf_hash;
    for (int i = 0; i < path_len; ++i) {
        HashWriter ss_branch{HASHER_TAPBRANCH};
        Span node{Span{control}.subspan(TAPROOT_CONTROL_BASE_SIZE + TAPROOT_CONTROL_NODE_SIZE * i, TAPROOT_CONTROL_NODE_SIZE)};
        if (std::lexicographical_compare(k.begin(), k.end(), node.begin(), node.end())) {
            ss_branch << k << node;
        } else {
            ss_branch << node << k;
        }
        k = ss_branch.GetSHA256();
    }
    return k;
}
static bool VerifyTaprootCommitment(const std::vector<unsigned char>& control, const std::vector<unsigned char>& program, const uint256& tapleaf_hash)
{
    assert(control.size() >= TAPROOT_CONTROL_BASE_SIZE);
    assert(program.size() >= uint256::size());
    const XOnlyPubKey p{Span{control}.subspan(1, TAPROOT_CONTROL_BASE_SIZE - 1)};
    const XOnlyPubKey q{program};
    const uint256 merkle_root = ComputeTaprootMerkleRoot(control, tapleaf_hash);
    return q.CheckTapTweak(p, merkle_root, control[0] & 1);
}
static bool VerifyWitnessProgram(const CScriptWitness& witness, int witversion, const std::vector<unsigned char>& program, unsigned int flags, const BaseSignatureChecker& checker, ScriptError* serror, bool is_p2sh)
{
    CScript exec_script;
    Span stack{witness.stack};
    ScriptExecutionData execdata;
    if (witversion == 0) {
        if (program.size() == WITNESS_V0_SCRIPTHASH_SIZE) {
            if (stack.size() == 0) {
                return set_error(serror, SCRIPT_ERR_WITNESS_PROGRAM_WITNESS_EMPTY);
            }
            const valtype& script_bytes = SpanPopBack(stack);
            exec_script = CScript(script_bytes.begin(), script_bytes.end());
            uint256 hash_exec_script;
            CSHA256().Write(exec_script.data(), exec_script.size()).Finalize(hash_exec_script.begin());
            if (memcmp(hash_exec_script.begin(), program.data(), 32)) {
                return set_error(serror, SCRIPT_ERR_WITNESS_PROGRAM_MISMATCH);
            }
            return ExecuteWitnessScript(stack, exec_script, flags, SigVersion::WITNESS_V0, checker, execdata, serror);
        } else if (program.size() == WITNESS_V0_KEYHASH_SIZE) {
            if (stack.size() != 2) {
                return set_error(serror, SCRIPT_ERR_WITNESS_PROGRAM_MISMATCH);
            }
            exec_script << OP_DUP << OP_HASH160 << program << OP_EQUALVERIFY << OP_CHECKSIG;
            return ExecuteWitnessScript(stack, exec_script, flags, SigVersion::WITNESS_V0, checker, execdata, serror);
        } else {
            return set_error(serror, SCRIPT_ERR_WITNESS_PROGRAM_WRONG_LENGTH);
        }
    } else if (witversion == 1 && program.size() == WITNESS_V1_TAPROOT_SIZE && !is_p2sh) {
        if (!(flags & SCRIPT_VERIFY_TAPROOT)) return set_success(serror);
        if (stack.size() == 0) return set_error(serror, SCRIPT_ERR_WITNESS_PROGRAM_WITNESS_EMPTY);
        if (stack.size() >= 2 && !stack.back().empty() && stack.back()[0] == ANNEX_TAG) {
            const valtype& annex = SpanPopBack(stack);
            execdata.m_annex_hash = (HashWriter{} << annex).GetSHA256();
            execdata.m_annex_present = true;
        } else {
            execdata.m_annex_present = false;
        }
        execdata.m_annex_init = true;
        if (stack.size() == 1) {
            if (!checker.CheckSchnorrSignature(stack.front(), program, SigVersion::TAPROOT, execdata, serror)) {
                return false;
            }
            return set_success(serror);
        } else {
            const valtype& control = SpanPopBack(stack);
            const valtype& script_bytes = SpanPopBack(stack);
            exec_script = CScript(script_bytes.begin(), script_bytes.end());
            if (control.size() < TAPROOT_CONTROL_BASE_SIZE || control.size() > TAPROOT_CONTROL_MAX_SIZE || ((control.size() - TAPROOT_CONTROL_BASE_SIZE) % TAPROOT_CONTROL_NODE_SIZE) != 0) {
                return set_error(serror, SCRIPT_ERR_TAPROOT_WRONG_CONTROL_SIZE);
            }
            execdata.m_tapleaf_hash = ComputeTapleafHash(control[0] & TAPROOT_LEAF_MASK, exec_script);
            if (!VerifyTaprootCommitment(control, program, execdata.m_tapleaf_hash)) {
                return set_error(serror, SCRIPT_ERR_WITNESS_PROGRAM_MISMATCH);
            }
            execdata.m_tapleaf_hash_init = true;
            if ((control[0] & TAPROOT_LEAF_MASK) == TAPROOT_LEAF_TAPSCRIPT) {
                execdata.m_validation_weight_left = ::GetSerializeSize(witness.stack, PROTOCOL_VERSION) + VALIDATION_WEIGHT_OFFSET;
                execdata.m_validation_weight_left_init = true;
                return ExecuteWitnessScript(stack, exec_script, flags, SigVersion::TAPSCRIPT, checker, execdata, serror);
            }
            if (flags & SCRIPT_VERIFY_DISCOURAGE_UPGRADABLE_TAPROOT_VERSION) {
                return set_error(serror, SCRIPT_ERR_DISCOURAGE_UPGRADABLE_TAPROOT_VERSION);
            }
            return set_success(serror);
        }
    } else {
        if (flags & SCRIPT_VERIFY_DISCOURAGE_UPGRADABLE_WITNESS_PROGRAM) {
            return set_error(serror, SCRIPT_ERR_DISCOURAGE_UPGRADABLE_WITNESS_PROGRAM);
        }
        return true;
    }
}
bool VerifyScript(const CScript& scriptSig, const CScript& scriptPubKey, const CScriptWitness* witness, unsigned int flags, const BaseSignatureChecker& checker, ScriptError* serror)
{
    static const CScriptWitness emptyWitness;
    if (witness == nullptr) {
        witness = &emptyWitness;
    }
    bool hadWitness = false;
    set_error(serror, SCRIPT_ERR_UNKNOWN_ERROR);
    if ((flags & SCRIPT_VERIFY_SIGPUSHONLY) != 0 && !scriptSig.IsPushOnly()) {
        return set_error(serror, SCRIPT_ERR_SIG_PUSHONLY);
    }
    std::vector<std::vector<unsigned char> > stack, stackCopy;
    if (!EvalScript(stack, scriptSig, flags, checker, SigVersion::BASE, serror))
        return false;
    if (flags & SCRIPT_VERIFY_P2SH)
        stackCopy = stack;
    if (!EvalScript(stack, scriptPubKey, flags, checker, SigVersion::BASE, serror))
        return false;
    if (stack.empty())
        return set_error(serror, SCRIPT_ERR_EVAL_FALSE);
    if (CastToBool(stack.back()) == false)
        return set_error(serror, SCRIPT_ERR_EVAL_FALSE);
    int witnessversion;
    std::vector<unsigned char> witnessprogram;
    if (flags & SCRIPT_VERIFY_WITNESS) {
        if (scriptPubKey.IsWitnessProgram(witnessversion, witnessprogram)) {
            hadWitness = true;
            if (scriptSig.size() != 0) {
                return set_error(serror, SCRIPT_ERR_WITNESS_MALLEATED);
            }
            if (!VerifyWitnessProgram(*witness, witnessversion, witnessprogram, flags, checker, serror,  false)) {
                return false;
            }
            stack.resize(1);
        }
    }
    if ((flags & SCRIPT_VERIFY_P2SH) && scriptPubKey.IsPayToScriptHash())
    {
        if (!scriptSig.IsPushOnly())
            return set_error(serror, SCRIPT_ERR_SIG_PUSHONLY);
        swap(stack, stackCopy);
        assert(!stack.empty());
        const valtype& pubKeySerialized = stack.back();
        CScript pubKey2(pubKeySerialized.begin(), pubKeySerialized.end());
        popstack(stack);
        if (!EvalScript(stack, pubKey2, flags, checker, SigVersion::BASE, serror))
            return false;
        if (stack.empty())
            return set_error(serror, SCRIPT_ERR_EVAL_FALSE);
        if (!CastToBool(stack.back()))
            return set_error(serror, SCRIPT_ERR_EVAL_FALSE);
        if (flags & SCRIPT_VERIFY_WITNESS) {
            if (pubKey2.IsWitnessProgram(witnessversion, witnessprogram)) {
                hadWitness = true;
                if (scriptSig != CScript() << std::vector<unsigned char>(pubKey2.begin(), pubKey2.end())) {
                    return set_error(serror, SCRIPT_ERR_WITNESS_MALLEATED_P2SH);
                }
                if (!VerifyWitnessProgram(*witness, witnessversion, witnessprogram, flags, checker, serror,  true)) {
                    return false;
                }
                stack.resize(1);
            }
        }
    }
    if ((flags & SCRIPT_VERIFY_CLEANSTACK) != 0) {
        assert((flags & SCRIPT_VERIFY_P2SH) != 0);
        assert((flags & SCRIPT_VERIFY_WITNESS) != 0);
        if (stack.size() != 1) {
            return set_error(serror, SCRIPT_ERR_CLEANSTACK);
        }
    }
    if (flags & SCRIPT_VERIFY_WITNESS) {
        assert((flags & SCRIPT_VERIFY_P2SH) != 0);
        if (!hadWitness && !witness->IsNull()) {
            return set_error(serror, SCRIPT_ERR_WITNESS_UNEXPECTED);
        }
    }
    return set_success(serror);
}
size_t static WitnessSigOps(int witversion, const std::vector<unsigned char>& witprogram, const CScriptWitness& witness)
{
    if (witversion == 0) {
        if (witprogram.size() == WITNESS_V0_KEYHASH_SIZE)
            return 1;
        if (witprogram.size() == WITNESS_V0_SCRIPTHASH_SIZE && witness.stack.size() > 0) {
            CScript subscript(witness.stack.back().begin(), witness.stack.back().end());
            return subscript.GetSigOpCount(true);
        }
    }
    return 0;
}
size_t CountWitnessSigOps(const CScript& scriptSig, const CScript& scriptPubKey, const CScriptWitness* witness, unsigned int flags)
{
    static const CScriptWitness witnessEmpty;
    if ((flags & SCRIPT_VERIFY_WITNESS) == 0) {
        return 0;
    }
    assert((flags & SCRIPT_VERIFY_P2SH) != 0);
    int witnessversion;
    std::vector<unsigned char> witnessprogram;
    if (scriptPubKey.IsWitnessProgram(witnessversion, witnessprogram)) {
        return WitnessSigOps(witnessversion, witnessprogram, witness ? *witness : witnessEmpty);
    }
    if (scriptPubKey.IsPayToScriptHash() && scriptSig.IsPushOnly()) {
        CScript::const_iterator pc = scriptSig.begin();
        std::vector<unsigned char> data;
        while (pc < scriptSig.end()) {
            opcodetype opcode;
            scriptSig.GetOp(pc, opcode, data);
        }
        CScript subscript(data.begin(), data.end());
        if (subscript.IsWitnessProgram(witnessversion, witnessprogram)) {
            return WitnessSigOps(witnessversion, witnessprogram, witness ? *witness : witnessEmpty);
        }
    }
    return 0;
}
