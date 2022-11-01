#include <script/standard.h>
#include <crypto/sha256.h>
#include <hash.h>
#include <pubkey.h>
#include <script/interpreter.h>
#include <script/script.h>
#include <util/strencodings.h>
#include <string>
typedef std::vector<unsigned char> valtype;
CScriptID::CScriptID(const CScript& in) : BaseHash(Hash160(in)) {}
CScriptID::CScriptID(const ScriptHash& in) : BaseHash(static_cast<uint160>(in)) {}
ScriptHash::ScriptHash(const CScript& in) : BaseHash(Hash160(in)) {}
ScriptHash::ScriptHash(const CScriptID& in) : BaseHash(static_cast<uint160>(in)) {}
PKHash::PKHash(const CPubKey& pubkey) : BaseHash(pubkey.GetID()) {}
PKHash::PKHash(const CKeyID& pubkey_id) : BaseHash(pubkey_id) {}
WitnessV0KeyHash::WitnessV0KeyHash(const CPubKey& pubkey) : BaseHash(pubkey.GetID()) {}
WitnessV0KeyHash::WitnessV0KeyHash(const PKHash& pubkey_hash) : BaseHash(static_cast<uint160>(pubkey_hash)) {}
CKeyID ToKeyID(const PKHash& key_hash)
{
    return CKeyID{static_cast<uint160>(key_hash)};
}
CKeyID ToKeyID(const WitnessV0KeyHash& key_hash)
{
    return CKeyID{static_cast<uint160>(key_hash)};
}
WitnessV0ScriptHash::WitnessV0ScriptHash(const CScript& in)
{
    CSHA256().Write(in.data(), in.size()).Finalize(begin());
}
std::string GetTxnOutputType(TxoutType t)
{
    switch (t) {
    case TxoutType::NONSTANDARD: return "nonstandard";
    case TxoutType::PUBKEY: return "pubkey";
    case TxoutType::PUBKEYHASH: return "pubkeyhash";
    case TxoutType::SCRIPTHASH: return "scripthash";
    case TxoutType::MULTISIG: return "multisig";
    case TxoutType::NULL_DATA: return "nulldata";
    case TxoutType::WITNESS_V0_KEYHASH: return "witness_v0_keyhash";
    case TxoutType::WITNESS_V0_SCRIPTHASH: return "witness_v0_scripthash";
    case TxoutType::WITNESS_V1_TAPROOT: return "witness_v1_taproot";
    case TxoutType::WITNESS_UNKNOWN: return "witness_unknown";
    }
    assert(false);
}
static bool MatchPayToPubkey(const CScript& script, valtype& pubkey)
{
    if (script.size() == CPubKey::SIZE + 2 && script[0] == CPubKey::SIZE && script.back() == OP_CHECKSIG) {
        pubkey = valtype(script.begin() + 1, script.begin() + CPubKey::SIZE + 1);
        return CPubKey::ValidSize(pubkey);
    }
    if (script.size() == CPubKey::COMPRESSED_SIZE + 2 && script[0] == CPubKey::COMPRESSED_SIZE && script.back() == OP_CHECKSIG) {
        pubkey = valtype(script.begin() + 1, script.begin() + CPubKey::COMPRESSED_SIZE + 1);
        return CPubKey::ValidSize(pubkey);
    }
    return false;
}
static bool MatchPayToPubkeyHash(const CScript& script, valtype& pubkeyhash)
{
    if (script.size() == 25 && script[0] == OP_DUP && script[1] == OP_HASH160 && script[2] == 20 && script[23] == OP_EQUALVERIFY && script[24] == OP_CHECKSIG) {
        pubkeyhash = valtype(script.begin () + 3, script.begin() + 23);
        return true;
    }
    return false;
}
static constexpr bool IsSmallInteger(opcodetype opcode)
{
    return opcode >= OP_1 && opcode <= OP_16;
}
static std::optional<int> GetScriptNumber(opcodetype opcode, valtype data, int min, int max)
{
    int count;
    if (IsSmallInteger(opcode)) {
        count = CScript::DecodeOP_N(opcode);
    } else if (IsPushdataOp(opcode)) {
        if (!CheckMinimalPush(data, opcode)) return {};
        try {
            count = CScriptNum(data,   true).getint();
        } catch (const scriptnum_error&) {
            return {};
        }
    } else {
        return {};
    }
    if (count < min || count > max) return {};
    return count;
}
static bool MatchMultisig(const CScript& script, int& required_sigs, std::vector<valtype>& pubkeys)
{
    opcodetype opcode;
    valtype data;
    CScript::const_iterator it = script.begin();
    if (script.size() < 1 || script.back() != OP_CHECKMULTISIG) return false;
    if (!script.GetOp(it, opcode, data)) return false;
    auto req_sigs = GetScriptNumber(opcode, data, 1, MAX_PUBKEYS_PER_MULTISIG);
    if (!req_sigs) return false;
    required_sigs = *req_sigs;
    while (script.GetOp(it, opcode, data) && CPubKey::ValidSize(data)) {
        pubkeys.emplace_back(std::move(data));
    }
    auto num_keys = GetScriptNumber(opcode, data, required_sigs, MAX_PUBKEYS_PER_MULTISIG);
    if (!num_keys) return false;
    if (pubkeys.size() != static_cast<unsigned long>(*num_keys)) return false;
    return (it + 1 == script.end());
}
std::optional<std::pair<int, std::vector<Span<const unsigned char>>>> MatchMultiA(const CScript& script)
{
    std::vector<Span<const unsigned char>> keyspans;
    if (script.size() == 0 || script[0] != 32 || script.back() != OP_NUMEQUAL) return {};
    auto it = script.begin();
    while (script.end() - it >= 34) {
        if (*it != 32) return {};
        ++it;
        keyspans.emplace_back(&*it, 32);
        it += 32;
        if (*it != (keyspans.size() == 1 ? OP_CHECKSIG : OP_CHECKSIGADD)) return {};
        ++it;
    }
    if (keyspans.size() == 0 || keyspans.size() > MAX_PUBKEYS_PER_MULTI_A) return {};
    opcodetype opcode;
    std::vector<unsigned char> data;
    if (!script.GetOp(it, opcode, data)) return {};
    if (it == script.end()) return {};
    if (*it != OP_NUMEQUAL) return {};
    ++it;
    if (it != script.end()) return {};
    auto threshold = GetScriptNumber(opcode, data, 1, (int)keyspans.size());
    if (!threshold) return {};
    return std::pair{*threshold, std::move(keyspans)};
}
TxoutType Solver(const CScript& scriptPubKey, std::vector<std::vector<unsigned char>>& vSolutionsRet)
{
    vSolutionsRet.clear();
    if (scriptPubKey.IsPayToScriptHash())
    {
        std::vector<unsigned char> hashBytes(scriptPubKey.begin()+2, scriptPubKey.begin()+22);
        vSolutionsRet.push_back(hashBytes);
        return TxoutType::SCRIPTHASH;
    }
    int witnessversion;
    std::vector<unsigned char> witnessprogram;
    if (scriptPubKey.IsWitnessProgram(witnessversion, witnessprogram)) {
        if (witnessversion == 0 && witnessprogram.size() == WITNESS_V0_KEYHASH_SIZE) {
            vSolutionsRet.push_back(std::move(witnessprogram));
            return TxoutType::WITNESS_V0_KEYHASH;
        }
        if (witnessversion == 0 && witnessprogram.size() == WITNESS_V0_SCRIPTHASH_SIZE) {
            vSolutionsRet.push_back(std::move(witnessprogram));
            return TxoutType::WITNESS_V0_SCRIPTHASH;
        }
        if (witnessversion == 1 && witnessprogram.size() == WITNESS_V1_TAPROOT_SIZE) {
            vSolutionsRet.push_back(std::move(witnessprogram));
            return TxoutType::WITNESS_V1_TAPROOT;
        }
        if (witnessversion != 0) {
            vSolutionsRet.push_back(std::vector<unsigned char>{(unsigned char)witnessversion});
            vSolutionsRet.push_back(std::move(witnessprogram));
            return TxoutType::WITNESS_UNKNOWN;
        }
        return TxoutType::NONSTANDARD;
    }
    if (scriptPubKey.size() >= 1 && scriptPubKey[0] == OP_RETURN && scriptPubKey.IsPushOnly(scriptPubKey.begin()+1)) {
        return TxoutType::NULL_DATA;
    }
    std::vector<unsigned char> data;
    if (MatchPayToPubkey(scriptPubKey, data)) {
        vSolutionsRet.push_back(std::move(data));
        return TxoutType::PUBKEY;
    }
    if (MatchPayToPubkeyHash(scriptPubKey, data)) {
        vSolutionsRet.push_back(std::move(data));
        return TxoutType::PUBKEYHASH;
    }
    int required;
    std::vector<std::vector<unsigned char>> keys;
    if (MatchMultisig(scriptPubKey, required, keys)) {
        vSolutionsRet.push_back({static_cast<unsigned char>(required)});
        vSolutionsRet.insert(vSolutionsRet.end(), keys.begin(), keys.end());
        vSolutionsRet.push_back({static_cast<unsigned char>(keys.size())});
        return TxoutType::MULTISIG;
    }
    vSolutionsRet.clear();
    return TxoutType::NONSTANDARD;
}
bool ExtractDestination(const CScript& scriptPubKey, CTxDestination& addressRet)
{
    std::vector<valtype> vSolutions;
    TxoutType whichType = Solver(scriptPubKey, vSolutions);
    switch (whichType) {
    case TxoutType::PUBKEY: {
        CPubKey pubKey(vSolutions[0]);
        if (!pubKey.IsValid())
            return false;
        addressRet = PKHash(pubKey);
        return true;
    }
    case TxoutType::PUBKEYHASH: {
        addressRet = PKHash(uint160(vSolutions[0]));
        return true;
    }
    case TxoutType::SCRIPTHASH: {
        addressRet = ScriptHash(uint160(vSolutions[0]));
        return true;
    }
    case TxoutType::WITNESS_V0_KEYHASH: {
        WitnessV0KeyHash hash;
        std::copy(vSolutions[0].begin(), vSolutions[0].end(), hash.begin());
        addressRet = hash;
        return true;
    }
    case TxoutType::WITNESS_V0_SCRIPTHASH: {
        WitnessV0ScriptHash hash;
        std::copy(vSolutions[0].begin(), vSolutions[0].end(), hash.begin());
        addressRet = hash;
        return true;
    }
    case TxoutType::WITNESS_V1_TAPROOT: {
        WitnessV1Taproot tap;
        std::copy(vSolutions[0].begin(), vSolutions[0].end(), tap.begin());
        addressRet = tap;
        return true;
    }
    case TxoutType::WITNESS_UNKNOWN: {
        WitnessUnknown unk;
        unk.version = vSolutions[0][0];
        std::copy(vSolutions[1].begin(), vSolutions[1].end(), unk.program);
        unk.length = vSolutions[1].size();
        addressRet = unk;
        return true;
    }
    case TxoutType::MULTISIG:
    case TxoutType::NULL_DATA:
    case TxoutType::NONSTANDARD:
        return false;
    }
    assert(false);
}
namespace {
class CScriptVisitor
{
public:
    CScript operator()(const CNoDestination& dest) const
    {
        return CScript();
    }
    CScript operator()(const PKHash& keyID) const
    {
        return CScript() << OP_DUP << OP_HASH160 << ToByteVector(keyID) << OP_EQUALVERIFY << OP_CHECKSIG;
    }
    CScript operator()(const ScriptHash& scriptID) const
    {
        return CScript() << OP_HASH160 << ToByteVector(scriptID) << OP_EQUAL;
    }
    CScript operator()(const WitnessV0KeyHash& id) const
    {
        return CScript() << OP_0 << ToByteVector(id);
    }
    CScript operator()(const WitnessV0ScriptHash& id) const
    {
        return CScript() << OP_0 << ToByteVector(id);
    }
    CScript operator()(const WitnessV1Taproot& tap) const
    {
        return CScript() << OP_1 << ToByteVector(tap);
    }
    CScript operator()(const WitnessUnknown& id) const
    {
        return CScript() << CScript::EncodeOP_N(id.version) << std::vector<unsigned char>(id.program, id.program + id.length);
    }
};
}
CScript GetScriptForDestination(const CTxDestination& dest)
{
    return std::visit(CScriptVisitor(), dest);
}
CScript GetScriptForRawPubKey(const CPubKey& pubKey)
{
    return CScript() << std::vector<unsigned char>(pubKey.begin(), pubKey.end()) << OP_CHECKSIG;
}
CScript GetScriptForMultisig(int nRequired, const std::vector<CPubKey>& keys)
{
    CScript script;
    script << nRequired;
    for (const CPubKey& key : keys)
        script << ToByteVector(key);
    script << keys.size() << OP_CHECKMULTISIG;
    return script;
}
bool IsValidDestination(const CTxDestination& dest) {
    return dest.index() != 0;
}
  TaprootBuilder::NodeInfo TaprootBuilder::Combine(NodeInfo&& a, NodeInfo&& b)
{
    NodeInfo ret;
    for (auto& leaf : a.leaves) {
        leaf.merkle_branch.push_back(b.hash);
        ret.leaves.emplace_back(std::move(leaf));
    }
    for (auto& leaf : b.leaves) {
        leaf.merkle_branch.push_back(a.hash);
        ret.leaves.emplace_back(std::move(leaf));
    }
    if (a.hash < b.hash) {
        ret.hash = (HashWriter{HASHER_TAPBRANCH} << a.hash << b.hash).GetSHA256();
    } else {
        ret.hash = (HashWriter{HASHER_TAPBRANCH} << b.hash << a.hash).GetSHA256();
    }
    return ret;
}
void TaprootSpendData::Merge(TaprootSpendData other)
{
    if (internal_key.IsNull() && !other.internal_key.IsNull()) {
        internal_key = other.internal_key;
    }
    if (merkle_root.IsNull() && !other.merkle_root.IsNull()) {
        merkle_root = other.merkle_root;
    }
    for (auto& [key, control_blocks] : other.scripts) {
        scripts[key].merge(std::move(control_blocks));
    }
}
void TaprootBuilder::Insert(TaprootBuilder::NodeInfo&& node, int depth)
{
    assert(depth >= 0 && (size_t)depth <= TAPROOT_CONTROL_MAX_NODE_COUNT);
    if ((size_t)depth + 1 < m_branch.size()) {
        m_valid = false;
        return;
    }
    while (m_valid && m_branch.size() > (size_t)depth && m_branch[depth].has_value()) {
        node = Combine(std::move(node), std::move(*m_branch[depth]));
        m_branch.pop_back();
        if (depth == 0) m_valid = false;
        --depth;
    }
    if (m_valid) {
        if (m_branch.size() <= (size_t)depth) m_branch.resize((size_t)depth + 1);
        assert(!m_branch[depth].has_value());
        m_branch[depth] = std::move(node);
    }
}
  bool TaprootBuilder::ValidDepths(const std::vector<int>& depths)
{
    std::vector<bool> branch;
    for (int depth : depths) {
        if (depth < 0 || (size_t)depth > TAPROOT_CONTROL_MAX_NODE_COUNT) return false;
        if ((size_t)depth + 1 < branch.size()) return false;
        while (branch.size() > (size_t)depth && branch[depth]) {
            branch.pop_back();
            if (depth == 0) return false;
            --depth;
        }
        if (branch.size() <= (size_t)depth) branch.resize((size_t)depth + 1);
        assert(!branch[depth]);
        branch[depth] = true;
    }
    return branch.size() == 0 || (branch.size() == 1 && branch[0]);
}
TaprootBuilder& TaprootBuilder::Add(int depth, const CScript& script, int leaf_version, bool track)
{
    assert((leaf_version & ~TAPROOT_LEAF_MASK) == 0);
    if (!IsValid()) return *this;
    NodeInfo node;
    node.hash = (HashWriter{HASHER_TAPLEAF} << uint8_t(leaf_version) << script).GetSHA256();
    if (track) node.leaves.emplace_back(LeafInfo{script, leaf_version, {}});
    Insert(std::move(node), depth);
    return *this;
}
TaprootBuilder& TaprootBuilder::AddOmitted(int depth, const uint256& hash)
{
    if (!IsValid()) return *this;
    NodeInfo node;
    node.hash = hash;
    Insert(std::move(node), depth);
    return *this;
}
TaprootBuilder& TaprootBuilder::Finalize(const XOnlyPubKey& internal_key)
{
    assert(IsComplete());
    m_internal_key = internal_key;
    auto ret = m_internal_key.CreateTapTweak(m_branch.size() == 0 ? nullptr : &m_branch[0]->hash);
    assert(ret.has_value());
    std::tie(m_output_key, m_parity) = *ret;
    return *this;
}
WitnessV1Taproot TaprootBuilder::GetOutput() { return WitnessV1Taproot{m_output_key}; }
TaprootSpendData TaprootBuilder::GetSpendData() const
{
    assert(IsComplete());
    assert(m_output_key.IsFullyValid());
    TaprootSpendData spd;
    spd.merkle_root = m_branch.size() == 0 ? uint256() : m_branch[0]->hash;
    spd.internal_key = m_internal_key;
    if (m_branch.size()) {
        for (const auto& leaf : m_branch[0]->leaves) {
            std::vector<unsigned char> control_block;
            control_block.resize(TAPROOT_CONTROL_BASE_SIZE + TAPROOT_CONTROL_NODE_SIZE * leaf.merkle_branch.size());
            control_block[0] = leaf.leaf_version | (m_parity ? 1 : 0);
            std::copy(m_internal_key.begin(), m_internal_key.end(), control_block.begin() + 1);
            if (leaf.merkle_branch.size()) {
                std::copy(leaf.merkle_branch[0].begin(),
                          leaf.merkle_branch[0].begin() + TAPROOT_CONTROL_NODE_SIZE * leaf.merkle_branch.size(),
                          control_block.begin() + TAPROOT_CONTROL_BASE_SIZE);
            }
            spd.scripts[{leaf.script, leaf.leaf_version}].insert(std::move(control_block));
        }
    }
    return spd;
}
std::optional<std::vector<std::tuple<int, CScript, int>>> InferTaprootTree(const TaprootSpendData& spenddata, const XOnlyPubKey& output)
{
    auto tweak = spenddata.internal_key.CreateTapTweak(spenddata.merkle_root.IsNull() ? nullptr : &spenddata.merkle_root);
    if (!tweak || tweak->first != output) return std::nullopt;
    std::vector<std::tuple<int, CScript, int>> ret;
    if (spenddata.merkle_root.IsNull()) return ret;
    struct TreeNode {
        uint256 hash;
        std::unique_ptr<TreeNode> sub[2];
        const std::pair<CScript, int>* leaf = nullptr;
        bool explored = false;
        bool inner;
        bool done = false;
    };
    TreeNode root;
    root.hash = spenddata.merkle_root;
    for (const auto& [key, control_blocks] : spenddata.scripts) {
        const auto& [script, leaf_ver] = key;
        for (const auto& control : control_blocks) {
            if (leaf_ver < 0 || leaf_ver >= 0x100 || leaf_ver & 1) continue;
            if (control.size() < TAPROOT_CONTROL_BASE_SIZE || control.size() > TAPROOT_CONTROL_MAX_SIZE ||
                ((control.size() - TAPROOT_CONTROL_BASE_SIZE) % TAPROOT_CONTROL_NODE_SIZE) != 0) continue;
            if ((control[0] & TAPROOT_LEAF_MASK) != leaf_ver) continue;
            const uint256 leaf_hash = ComputeTapleafHash(leaf_ver, script);
            const uint256 merkle_root = ComputeTaprootMerkleRoot(control, leaf_hash);
            if (merkle_root != spenddata.merkle_root) continue;
            TreeNode* node = &root;
            size_t levels = (control.size() - TAPROOT_CONTROL_BASE_SIZE) / TAPROOT_CONTROL_NODE_SIZE;
            for (size_t depth = 0; depth < levels; ++depth) {
                if (node->explored && !node->inner) return std::nullopt;
                uint256 hash;
                std::copy(control.begin() + TAPROOT_CONTROL_BASE_SIZE + (levels - 1 - depth) * TAPROOT_CONTROL_NODE_SIZE,
                          control.begin() + TAPROOT_CONTROL_BASE_SIZE + (levels - depth) * TAPROOT_CONTROL_NODE_SIZE,
                          hash.begin());
                if (node->sub[0]) {
                    bool desc = false;
                    for (int i = 0; i < 2; ++i) {
                        if (node->sub[i]->hash == hash || (node->sub[i]->hash.IsNull() && node->sub[1-i]->hash != hash)) {
                            node->sub[i]->hash = hash;
                            node = &*node->sub[1-i];
                            desc = true;
                            break;
                        }
                    }
                    if (!desc) return std::nullopt;
                } else {
                    node->explored = true;
                    node->inner = true;
                    node->sub[0] = std::make_unique<TreeNode>();
                    node->sub[1] = std::make_unique<TreeNode>();
                    node->sub[1]->hash = hash;
                    node = &*node->sub[0];
                }
            }
            if (node->sub[0]) return std::nullopt;
            node->explored = true;
            node->inner = false;
            node->leaf = &key;
            node->hash = leaf_hash;
        }
    }
    std::vector<TreeNode*> stack{&root};
    while (!stack.empty()) {
        TreeNode& node = *stack.back();
        if (!node.explored) {
            return std::nullopt;
        } else if (!node.inner) {
            ret.emplace_back(stack.size() - 1, node.leaf->first, node.leaf->second);
            node.done = true;
            stack.pop_back();
        } else if (node.sub[0]->done && !node.sub[1]->done && !node.sub[1]->explored && !node.sub[1]->hash.IsNull() &&
                   (HashWriter{HASHER_TAPBRANCH} << node.sub[1]->hash << node.sub[1]->hash).GetSHA256() == node.hash) {
            node.sub[0]->done = false;
            node.sub[1]->done = true;
        } else if (node.sub[0]->done && node.sub[1]->done) {
            node.sub[0]->done = false;
            node.sub[1]->done = false;
            node.done = true;
            stack.pop_back();
        } else if (!node.sub[0]->done) {
            stack.push_back(&*node.sub[0]);
        } else if (!node.sub[1]->done) {
            stack.push_back(&*node.sub[1]);
        }
    }
    return ret;
}
std::vector<std::tuple<uint8_t, uint8_t, CScript>> TaprootBuilder::GetTreeTuples() const
{
    assert(IsComplete());
    std::vector<std::tuple<uint8_t, uint8_t, CScript>> tuples;
    if (m_branch.size()) {
        const auto& leaves = m_branch[0]->leaves;
        for (const auto& leaf : leaves) {
            assert(leaf.merkle_branch.size() <= TAPROOT_CONTROL_MAX_NODE_COUNT);
            uint8_t depth = (uint8_t)leaf.merkle_branch.size();
            uint8_t leaf_ver = (uint8_t)leaf.leaf_version;
            tuples.push_back(std::make_tuple(depth, leaf_ver, leaf.script));
        }
    }
    return tuples;
}
