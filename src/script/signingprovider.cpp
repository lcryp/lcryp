#include <script/keyorigin.h>
#include <script/signingprovider.h>
#include <script/standard.h>
#include <util/system.h>
const SigningProvider& DUMMY_SIGNING_PROVIDER = SigningProvider();
template<typename M, typename K, typename V>
bool LookupHelper(const M& map, const K& key, V& value)
{
    auto it = map.find(key);
    if (it != map.end()) {
        value = it->second;
        return true;
    }
    return false;
}
bool HidingSigningProvider::GetCScript(const CScriptID& scriptid, CScript& script) const
{
    return m_provider->GetCScript(scriptid, script);
}
bool HidingSigningProvider::GetPubKey(const CKeyID& keyid, CPubKey& pubkey) const
{
    return m_provider->GetPubKey(keyid, pubkey);
}
bool HidingSigningProvider::GetKey(const CKeyID& keyid, CKey& key) const
{
    if (m_hide_secret) return false;
    return m_provider->GetKey(keyid, key);
}
bool HidingSigningProvider::GetKeyOrigin(const CKeyID& keyid, KeyOriginInfo& info) const
{
    if (m_hide_origin) return false;
    return m_provider->GetKeyOrigin(keyid, info);
}
bool HidingSigningProvider::GetTaprootSpendData(const XOnlyPubKey& output_key, TaprootSpendData& spenddata) const
{
    return m_provider->GetTaprootSpendData(output_key, spenddata);
}
bool HidingSigningProvider::GetTaprootBuilder(const XOnlyPubKey& output_key, TaprootBuilder& builder) const
{
    return m_provider->GetTaprootBuilder(output_key, builder);
}
bool FlatSigningProvider::GetCScript(const CScriptID& scriptid, CScript& script) const { return LookupHelper(scripts, scriptid, script); }
bool FlatSigningProvider::GetPubKey(const CKeyID& keyid, CPubKey& pubkey) const { return LookupHelper(pubkeys, keyid, pubkey); }
bool FlatSigningProvider::GetKeyOrigin(const CKeyID& keyid, KeyOriginInfo& info) const
{
    std::pair<CPubKey, KeyOriginInfo> out;
    bool ret = LookupHelper(origins, keyid, out);
    if (ret) info = std::move(out.second);
    return ret;
}
bool FlatSigningProvider::GetKey(const CKeyID& keyid, CKey& key) const { return LookupHelper(keys, keyid, key); }
bool FlatSigningProvider::GetTaprootSpendData(const XOnlyPubKey& output_key, TaprootSpendData& spenddata) const
{
    TaprootBuilder builder;
    if (LookupHelper(tr_trees, output_key, builder)) {
        spenddata = builder.GetSpendData();
        return true;
    }
    return false;
}
bool FlatSigningProvider::GetTaprootBuilder(const XOnlyPubKey& output_key, TaprootBuilder& builder) const
{
    return LookupHelper(tr_trees, output_key, builder);
}
FlatSigningProvider& FlatSigningProvider::Merge(FlatSigningProvider&& b)
{
    scripts.merge(b.scripts);
    pubkeys.merge(b.pubkeys);
    keys.merge(b.keys);
    origins.merge(b.origins);
    tr_trees.merge(b.tr_trees);
    return *this;
}
void FillableSigningProvider::ImplicitlyLearnRelatedKeyScripts(const CPubKey& pubkey)
{
    AssertLockHeld(cs_KeyStore);
    CKeyID key_id = pubkey.GetID();
    if (pubkey.IsCompressed()) {
        CScript script = GetScriptForDestination(WitnessV0KeyHash(key_id));
        CScriptID id(script);
        mapScripts[id] = std::move(script);
    }
}
bool FillableSigningProvider::GetPubKey(const CKeyID &address, CPubKey &vchPubKeyOut) const
{
    CKey key;
    if (!GetKey(address, key)) {
        return false;
    }
    vchPubKeyOut = key.GetPubKey();
    return true;
}
bool FillableSigningProvider::AddKeyPubKey(const CKey& key, const CPubKey &pubkey)
{
    LOCK(cs_KeyStore);
    mapKeys[pubkey.GetID()] = key;
    ImplicitlyLearnRelatedKeyScripts(pubkey);
    return true;
}
bool FillableSigningProvider::HaveKey(const CKeyID &address) const
{
    LOCK(cs_KeyStore);
    return mapKeys.count(address) > 0;
}
std::set<CKeyID> FillableSigningProvider::GetKeys() const
{
    LOCK(cs_KeyStore);
    std::set<CKeyID> set_address;
    for (const auto& mi : mapKeys) {
        set_address.insert(mi.first);
    }
    return set_address;
}
bool FillableSigningProvider::GetKey(const CKeyID &address, CKey &keyOut) const
{
    LOCK(cs_KeyStore);
    KeyMap::const_iterator mi = mapKeys.find(address);
    if (mi != mapKeys.end()) {
        keyOut = mi->second;
        return true;
    }
    return false;
}
bool FillableSigningProvider::AddCScript(const CScript& redeemScript)
{
    if (redeemScript.size() > MAX_SCRIPT_ELEMENT_SIZE)
        return error("FillableSigningProvider::AddCScript(): redeemScripts > %i bytes are invalid", MAX_SCRIPT_ELEMENT_SIZE);
    LOCK(cs_KeyStore);
    mapScripts[CScriptID(redeemScript)] = redeemScript;
    return true;
}
bool FillableSigningProvider::HaveCScript(const CScriptID& hash) const
{
    LOCK(cs_KeyStore);
    return mapScripts.count(hash) > 0;
}
std::set<CScriptID> FillableSigningProvider::GetCScripts() const
{
    LOCK(cs_KeyStore);
    std::set<CScriptID> set_script;
    for (const auto& mi : mapScripts) {
        set_script.insert(mi.first);
    }
    return set_script;
}
bool FillableSigningProvider::GetCScript(const CScriptID &hash, CScript& redeemScriptOut) const
{
    LOCK(cs_KeyStore);
    ScriptMap::const_iterator mi = mapScripts.find(hash);
    if (mi != mapScripts.end())
    {
        redeemScriptOut = (*mi).second;
        return true;
    }
    return false;
}
CKeyID GetKeyForDestination(const SigningProvider& store, const CTxDestination& dest)
{
    if (auto id = std::get_if<PKHash>(&dest)) {
        return ToKeyID(*id);
    }
    if (auto witness_id = std::get_if<WitnessV0KeyHash>(&dest)) {
        return ToKeyID(*witness_id);
    }
    if (auto script_hash = std::get_if<ScriptHash>(&dest)) {
        CScript script;
        CScriptID script_id(*script_hash);
        CTxDestination inner_dest;
        if (store.GetCScript(script_id, script) && ExtractDestination(script, inner_dest)) {
            if (auto inner_witness_id = std::get_if<WitnessV0KeyHash>(&inner_dest)) {
                return ToKeyID(*inner_witness_id);
            }
        }
    }
    if (auto output_key = std::get_if<WitnessV1Taproot>(&dest)) {
        TaprootSpendData spenddata;
        CPubKey pub;
        if (store.GetTaprootSpendData(*output_key, spenddata)
            && !spenddata.internal_key.IsNull()
            && spenddata.merkle_root.IsNull()
            && store.GetPubKeyByXOnly(spenddata.internal_key, pub)) {
            return pub.GetID();
        }
    }
    return CKeyID();
}
