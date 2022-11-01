#ifndef LCRYP_SCRIPT_STANDARD_H
#define LCRYP_SCRIPT_STANDARD_H
#include <attributes.h>
#include <pubkey.h>
#include <script/interpreter.h>
#include <uint256.h>
#include <util/hash_type.h>
#include <map>
#include <string>
#include <variant>
static const bool DEFAULT_ACCEPT_DATACARRIER = true;
class CKeyID;
class CScript;
struct ScriptHash;
class CScriptID : public BaseHash<uint160>
{
public:
    CScriptID() : BaseHash() {}
    explicit CScriptID(const CScript& in);
    explicit CScriptID(const uint160& in) : BaseHash(in) {}
    explicit CScriptID(const ScriptHash& in);
};
static const unsigned int MAX_OP_RETURN_RELAY = 83;
static const unsigned int MANDATORY_SCRIPT_VERIFY_FLAGS = SCRIPT_VERIFY_P2SH;
enum class TxoutType {
    NONSTANDARD,
    PUBKEY,
    PUBKEYHASH,
    SCRIPTHASH,
    MULTISIG,
    NULL_DATA,
    WITNESS_V0_SCRIPTHASH,
    WITNESS_V0_KEYHASH,
    WITNESS_V1_TAPROOT,
    WITNESS_UNKNOWN,
};
class CNoDestination {
public:
    friend bool operator==(const CNoDestination &a, const CNoDestination &b) { return true; }
    friend bool operator<(const CNoDestination &a, const CNoDestination &b) { return true; }
};
struct PKHash : public BaseHash<uint160>
{
    PKHash() : BaseHash() {}
    explicit PKHash(const uint160& hash) : BaseHash(hash) {}
    explicit PKHash(const CPubKey& pubkey);
    explicit PKHash(const CKeyID& pubkey_id);
};
CKeyID ToKeyID(const PKHash& key_hash);
struct WitnessV0KeyHash;
struct ScriptHash : public BaseHash<uint160>
{
    ScriptHash() : BaseHash() {}
    explicit ScriptHash(const WitnessV0KeyHash& hash) = delete;
    explicit ScriptHash(const PKHash& hash) = delete;
    explicit ScriptHash(const uint160& hash) : BaseHash(hash) {}
    explicit ScriptHash(const CScript& script);
    explicit ScriptHash(const CScriptID& script);
};
struct WitnessV0ScriptHash : public BaseHash<uint256>
{
    WitnessV0ScriptHash() : BaseHash() {}
    explicit WitnessV0ScriptHash(const uint256& hash) : BaseHash(hash) {}
    explicit WitnessV0ScriptHash(const CScript& script);
};
struct WitnessV0KeyHash : public BaseHash<uint160>
{
    WitnessV0KeyHash() : BaseHash() {}
    explicit WitnessV0KeyHash(const uint160& hash) : BaseHash(hash) {}
    explicit WitnessV0KeyHash(const CPubKey& pubkey);
    explicit WitnessV0KeyHash(const PKHash& pubkey_hash);
};
CKeyID ToKeyID(const WitnessV0KeyHash& key_hash);
struct WitnessV1Taproot : public XOnlyPubKey
{
    WitnessV1Taproot() : XOnlyPubKey() {}
    explicit WitnessV1Taproot(const XOnlyPubKey& xpk) : XOnlyPubKey(xpk) {}
};
struct WitnessUnknown
{
    unsigned int version;
    unsigned int length;
    unsigned char program[40];
    friend bool operator==(const WitnessUnknown& w1, const WitnessUnknown& w2) {
        if (w1.version != w2.version) return false;
        if (w1.length != w2.length) return false;
        return std::equal(w1.program, w1.program + w1.length, w2.program);
    }
    friend bool operator<(const WitnessUnknown& w1, const WitnessUnknown& w2) {
        if (w1.version < w2.version) return true;
        if (w1.version > w2.version) return false;
        if (w1.length < w2.length) return true;
        if (w1.length > w2.length) return false;
        return std::lexicographical_compare(w1.program, w1.program + w1.length, w2.program, w2.program + w2.length);
    }
};
using CTxDestination = std::variant<CNoDestination, PKHash, ScriptHash, WitnessV0ScriptHash, WitnessV0KeyHash, WitnessV1Taproot, WitnessUnknown>;
bool IsValidDestination(const CTxDestination& dest);
std::string GetTxnOutputType(TxoutType t);
constexpr bool IsPushdataOp(opcodetype opcode)
{
    return opcode > OP_FALSE && opcode <= OP_PUSHDATA4;
}
TxoutType Solver(const CScript& scriptPubKey, std::vector<std::vector<unsigned char>>& vSolutionsRet);
bool ExtractDestination(const CScript& scriptPubKey, CTxDestination& addressRet);
CScript GetScriptForDestination(const CTxDestination& dest);
CScript GetScriptForRawPubKey(const CPubKey& pubkey);
std::optional<std::pair<int, std::vector<Span<const unsigned char>>>> MatchMultiA(const CScript& script LIFETIMEBOUND);
CScript GetScriptForMultisig(int nRequired, const std::vector<CPubKey>& keys);
struct ShortestVectorFirstComparator
{
    bool operator()(const std::vector<unsigned char>& a, const std::vector<unsigned char>& b) const
    {
        if (a.size() < b.size()) return true;
        if (a.size() > b.size()) return false;
        return a < b;
    }
};
struct TaprootSpendData
{
    XOnlyPubKey internal_key;
    uint256 merkle_root;
    std::map<std::pair<CScript, int>, std::set<std::vector<unsigned char>, ShortestVectorFirstComparator>> scripts;
    void Merge(TaprootSpendData other);
};
class TaprootBuilder
{
private:
    struct LeafInfo
    {
        CScript script;
        int leaf_version;
        std::vector<uint256> merkle_branch;
    };
    struct NodeInfo
    {
        uint256 hash;
        std::vector<LeafInfo> leaves;
    };
    bool m_valid = true;
    std::vector<std::optional<NodeInfo>> m_branch;
    XOnlyPubKey m_internal_key;
    XOnlyPubKey m_output_key;
    bool m_parity;
    static NodeInfo Combine(NodeInfo&& a, NodeInfo&& b);
    void Insert(NodeInfo&& node, int depth);
public:
    TaprootBuilder& Add(int depth, const CScript& script, int leaf_version, bool track = true);
    TaprootBuilder& AddOmitted(int depth, const uint256& hash);
    TaprootBuilder& Finalize(const XOnlyPubKey& internal_key);
    bool IsValid() const { return m_valid; }
    bool IsComplete() const { return m_valid && (m_branch.size() == 0 || (m_branch.size() == 1 && m_branch[0].has_value())); }
    WitnessV1Taproot GetOutput();
    static bool ValidDepths(const std::vector<int>& depths);
    TaprootSpendData GetSpendData() const;
    std::vector<std::tuple<uint8_t, uint8_t, CScript>> GetTreeTuples() const;
};
std::optional<std::vector<std::tuple<int, CScript, int>>> InferTaprootTree(const TaprootSpendData& spenddata, const XOnlyPubKey& output);
#endif
