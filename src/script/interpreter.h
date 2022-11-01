#ifndef LCRYP_SCRIPT_INTERPRETER_H
#define LCRYP_SCRIPT_INTERPRETER_H
#include <hash.h>
#include <script/script_error.h>
#include <span.h>
#include <primitives/transaction.h>
#include <optional>
#include <vector>
#include <stdint.h>
class CPubKey;
class XOnlyPubKey;
class CScript;
class CTransaction;
class CTxOut;
class uint256;
enum
{
    SIGHASH_ALL = 1,
    SIGHASH_NONE = 2,
    SIGHASH_SINGLE = 3,
    SIGHASH_ANYONECANPAY = 0x80,
    SIGHASH_DEFAULT = 0,
    SIGHASH_OUTPUT_MASK = 3,
    SIGHASH_INPUT_MASK = 0x80,
};
enum : uint32_t {
    SCRIPT_VERIFY_NONE      = 0,
    SCRIPT_VERIFY_P2SH      = (1U << 0),
    SCRIPT_VERIFY_STRICTENC = (1U << 1),
    SCRIPT_VERIFY_DERSIG    = (1U << 2),
    SCRIPT_VERIFY_LOW_S     = (1U << 3),
    SCRIPT_VERIFY_NULLDUMMY = (1U << 4),
    SCRIPT_VERIFY_SIGPUSHONLY = (1U << 5),
    SCRIPT_VERIFY_MINIMALDATA = (1U << 6),
    SCRIPT_VERIFY_DISCOURAGE_UPGRADABLE_NOPS  = (1U << 7),
    SCRIPT_VERIFY_CLEANSTACK = (1U << 8),
    SCRIPT_VERIFY_CHECKLOCKTIMEVERIFY = (1U << 9),
    SCRIPT_VERIFY_CHECKSEQUENCEVERIFY = (1U << 10),
    SCRIPT_VERIFY_WITNESS = (1U << 11),
    SCRIPT_VERIFY_DISCOURAGE_UPGRADABLE_WITNESS_PROGRAM = (1U << 12),
    SCRIPT_VERIFY_MINIMALIF = (1U << 13),
    SCRIPT_VERIFY_NULLFAIL = (1U << 14),
    SCRIPT_VERIFY_WITNESS_PUBKEYTYPE = (1U << 15),
    SCRIPT_VERIFY_CONST_SCRIPTCODE = (1U << 16),
    SCRIPT_VERIFY_TAPROOT = (1U << 17),
    SCRIPT_VERIFY_DISCOURAGE_UPGRADABLE_TAPROOT_VERSION = (1U << 18),
    SCRIPT_VERIFY_DISCOURAGE_OP_SUCCESS = (1U << 19),
    SCRIPT_VERIFY_DISCOURAGE_UPGRADABLE_PUBKEYTYPE = (1U << 20),
    SCRIPT_VERIFY_END_MARKER
};
bool CheckSignatureEncoding(const std::vector<unsigned char> &vchSig, unsigned int flags, ScriptError* serror);
struct PrecomputedTransactionData
{
    uint256 m_prevouts_single_hash;
    uint256 m_sequences_single_hash;
    uint256 m_outputs_single_hash;
    uint256 m_spent_amounts_single_hash;
    uint256 m_spent_scripts_single_hash;
    bool m_bip341_taproot_ready = false;
    uint256 hashPrevouts, hashSequence, hashOutputs;
    bool m_bip143_segwit_ready = false;
    std::vector<CTxOut> m_spent_outputs;
    bool m_spent_outputs_ready = false;
    PrecomputedTransactionData() = default;
    template <class T>
    void Init(const T& tx, std::vector<CTxOut>&& spent_outputs, bool force = false);
    template <class T>
    explicit PrecomputedTransactionData(const T& tx);
};
enum class SigVersion
{
    BASE = 0,
    WITNESS_V0 = 1,
    TAPROOT = 2,
    TAPSCRIPT = 3,
};
struct ScriptExecutionData
{
    bool m_tapleaf_hash_init = false;
    uint256 m_tapleaf_hash;
    bool m_codeseparator_pos_init = false;
    uint32_t m_codeseparator_pos;
    bool m_annex_init = false;
    bool m_annex_present;
    uint256 m_annex_hash;
    bool m_validation_weight_left_init = false;
    int64_t m_validation_weight_left;
    std::optional<uint256> m_output_hash;
};
static constexpr size_t WITNESS_V0_SCRIPTHASH_SIZE = 32;
static constexpr size_t WITNESS_V0_KEYHASH_SIZE = 20;
static constexpr size_t WITNESS_V1_TAPROOT_SIZE = 32;
static constexpr uint8_t TAPROOT_LEAF_MASK = 0xfe;
static constexpr uint8_t TAPROOT_LEAF_TAPSCRIPT = 0xc0;
static constexpr size_t TAPROOT_CONTROL_BASE_SIZE = 33;
static constexpr size_t TAPROOT_CONTROL_NODE_SIZE = 32;
static constexpr size_t TAPROOT_CONTROL_MAX_NODE_COUNT = 128;
static constexpr size_t TAPROOT_CONTROL_MAX_SIZE = TAPROOT_CONTROL_BASE_SIZE + TAPROOT_CONTROL_NODE_SIZE * TAPROOT_CONTROL_MAX_NODE_COUNT;
extern const HashWriter HASHER_TAPSIGHASH;
extern const HashWriter HASHER_TAPLEAF;
extern const HashWriter HASHER_TAPBRANCH;
template <class T>
uint256 SignatureHash(const CScript& scriptCode, const T& txTo, unsigned int nIn, int nHashType, const CAmount& amount, SigVersion sigversion, const PrecomputedTransactionData* cache = nullptr);
class BaseSignatureChecker
{
public:
    virtual bool CheckECDSASignature(const std::vector<unsigned char>& scriptSig, const std::vector<unsigned char>& vchPubKey, const CScript& scriptCode, SigVersion sigversion) const
    {
        return false;
    }
    virtual bool CheckSchnorrSignature(Span<const unsigned char> sig, Span<const unsigned char> pubkey, SigVersion sigversion, ScriptExecutionData& execdata, ScriptError* serror = nullptr) const
    {
        return false;
    }
    virtual bool CheckLockTime(const CScriptNum& nLockTime) const
    {
         return false;
    }
    virtual bool CheckSequence(const CScriptNum& nSequence) const
    {
         return false;
    }
    virtual ~BaseSignatureChecker() {}
};
enum class MissingDataBehavior
{
    ASSERT_FAIL,
    FAIL,
};
template<typename T>
bool SignatureHashSchnorr(uint256& hash_out, ScriptExecutionData& execdata, const T& tx_to, uint32_t in_pos, uint8_t hash_type, SigVersion sigversion, const PrecomputedTransactionData& cache, MissingDataBehavior mdb);
template <class T>
class GenericTransactionSignatureChecker : public BaseSignatureChecker
{
private:
    const T* txTo;
    const MissingDataBehavior m_mdb;
    unsigned int nIn;
    const CAmount amount;
    const PrecomputedTransactionData* txdata;
protected:
    virtual bool VerifyECDSASignature(const std::vector<unsigned char>& vchSig, const CPubKey& vchPubKey, const uint256& sighash) const;
    virtual bool VerifySchnorrSignature(Span<const unsigned char> sig, const XOnlyPubKey& pubkey, const uint256& sighash) const;
public:
    GenericTransactionSignatureChecker(const T* txToIn, unsigned int nInIn, const CAmount& amountIn, MissingDataBehavior mdb) : txTo(txToIn), m_mdb(mdb), nIn(nInIn), amount(amountIn), txdata(nullptr) {}
    GenericTransactionSignatureChecker(const T* txToIn, unsigned int nInIn, const CAmount& amountIn, const PrecomputedTransactionData& txdataIn, MissingDataBehavior mdb) : txTo(txToIn), m_mdb(mdb), nIn(nInIn), amount(amountIn), txdata(&txdataIn) {}
    bool CheckECDSASignature(const std::vector<unsigned char>& scriptSig, const std::vector<unsigned char>& vchPubKey, const CScript& scriptCode, SigVersion sigversion) const override;
    bool CheckSchnorrSignature(Span<const unsigned char> sig, Span<const unsigned char> pubkey, SigVersion sigversion, ScriptExecutionData& execdata, ScriptError* serror = nullptr) const override;
    bool CheckLockTime(const CScriptNum& nLockTime) const override;
    bool CheckSequence(const CScriptNum& nSequence) const override;
};
using TransactionSignatureChecker = GenericTransactionSignatureChecker<CTransaction>;
using MutableTransactionSignatureChecker = GenericTransactionSignatureChecker<CMutableTransaction>;
class DeferringSignatureChecker : public BaseSignatureChecker
{
protected:
    const BaseSignatureChecker& m_checker;
public:
    DeferringSignatureChecker(const BaseSignatureChecker& checker) : m_checker(checker) {}
    bool CheckECDSASignature(const std::vector<unsigned char>& scriptSig, const std::vector<unsigned char>& vchPubKey, const CScript& scriptCode, SigVersion sigversion) const override
    {
        return m_checker.CheckECDSASignature(scriptSig, vchPubKey, scriptCode, sigversion);
    }
    bool CheckSchnorrSignature(Span<const unsigned char> sig, Span<const unsigned char> pubkey, SigVersion sigversion, ScriptExecutionData& execdata, ScriptError* serror = nullptr) const override
    {
        return m_checker.CheckSchnorrSignature(sig, pubkey, sigversion, execdata, serror);
    }
    bool CheckLockTime(const CScriptNum& nLockTime) const override
    {
        return m_checker.CheckLockTime(nLockTime);
    }
    bool CheckSequence(const CScriptNum& nSequence) const override
    {
        return m_checker.CheckSequence(nSequence);
    }
};
uint256 ComputeTapleafHash(uint8_t leaf_version, const CScript& script);
uint256 ComputeTaprootMerkleRoot(Span<const unsigned char> control, const uint256& tapleaf_hash);
bool EvalScript(std::vector<std::vector<unsigned char> >& stack, const CScript& script, unsigned int flags, const BaseSignatureChecker& checker, SigVersion sigversion, ScriptExecutionData& execdata, ScriptError* error = nullptr);
bool EvalScript(std::vector<std::vector<unsigned char> >& stack, const CScript& script, unsigned int flags, const BaseSignatureChecker& checker, SigVersion sigversion, ScriptError* error = nullptr);
bool VerifyScript(const CScript& scriptSig, const CScript& scriptPubKey, const CScriptWitness* witness, unsigned int flags, const BaseSignatureChecker& checker, ScriptError* serror = nullptr);
size_t CountWitnessSigOps(const CScript& scriptSig, const CScript& scriptPubKey, const CScriptWitness* witness, unsigned int flags);
int FindAndDelete(CScript& script, const CScript& b);
#endif
