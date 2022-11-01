#ifndef LCRYP_SCRIPT_SIGNINGPROVIDER_H
#define LCRYP_SCRIPT_SIGNINGPROVIDER_H
#include <attributes.h>
#include <key.h>
#include <pubkey.h>
#include <script/keyorigin.h>
#include <script/script.h>
#include <script/standard.h>
#include <sync.h>
class SigningProvider
{
public:
    virtual ~SigningProvider() {}
    virtual bool GetCScript(const CScriptID &scriptid, CScript& script) const { return false; }
    virtual bool HaveCScript(const CScriptID &scriptid) const { return false; }
    virtual bool GetPubKey(const CKeyID &address, CPubKey& pubkey) const { return false; }
    virtual bool GetKey(const CKeyID &address, CKey& key) const { return false; }
    virtual bool HaveKey(const CKeyID &address) const { return false; }
    virtual bool GetKeyOrigin(const CKeyID& keyid, KeyOriginInfo& info) const { return false; }
    virtual bool GetTaprootSpendData(const XOnlyPubKey& output_key, TaprootSpendData& spenddata) const { return false; }
    virtual bool GetTaprootBuilder(const XOnlyPubKey& output_key, TaprootBuilder& builder) const { return false; }
    bool GetKeyByXOnly(const XOnlyPubKey& pubkey, CKey& key) const
    {
        for (const auto& id : pubkey.GetKeyIDs()) {
            if (GetKey(id, key)) return true;
        }
        return false;
    }
    bool GetPubKeyByXOnly(const XOnlyPubKey& pubkey, CPubKey& out) const
    {
        for (const auto& id : pubkey.GetKeyIDs()) {
            if (GetPubKey(id, out)) return true;
        }
        return false;
    }
    bool GetKeyOriginByXOnly(const XOnlyPubKey& pubkey, KeyOriginInfo& info) const
    {
        for (const auto& id : pubkey.GetKeyIDs()) {
            if (GetKeyOrigin(id, info)) return true;
        }
        return false;
    }
};
extern const SigningProvider& DUMMY_SIGNING_PROVIDER;
class HidingSigningProvider : public SigningProvider
{
private:
    const bool m_hide_secret;
    const bool m_hide_origin;
    const SigningProvider* m_provider;
public:
    HidingSigningProvider(const SigningProvider* provider, bool hide_secret, bool hide_origin) : m_hide_secret(hide_secret), m_hide_origin(hide_origin), m_provider(provider) {}
    bool GetCScript(const CScriptID& scriptid, CScript& script) const override;
    bool GetPubKey(const CKeyID& keyid, CPubKey& pubkey) const override;
    bool GetKey(const CKeyID& keyid, CKey& key) const override;
    bool GetKeyOrigin(const CKeyID& keyid, KeyOriginInfo& info) const override;
    bool GetTaprootSpendData(const XOnlyPubKey& output_key, TaprootSpendData& spenddata) const override;
    bool GetTaprootBuilder(const XOnlyPubKey& output_key, TaprootBuilder& builder) const override;
};
struct FlatSigningProvider final : public SigningProvider
{
    std::map<CScriptID, CScript> scripts;
    std::map<CKeyID, CPubKey> pubkeys;
    std::map<CKeyID, std::pair<CPubKey, KeyOriginInfo>> origins;
    std::map<CKeyID, CKey> keys;
    std::map<XOnlyPubKey, TaprootBuilder> tr_trees;
    bool GetCScript(const CScriptID& scriptid, CScript& script) const override;
    bool GetPubKey(const CKeyID& keyid, CPubKey& pubkey) const override;
    bool GetKeyOrigin(const CKeyID& keyid, KeyOriginInfo& info) const override;
    bool GetKey(const CKeyID& keyid, CKey& key) const override;
    bool GetTaprootSpendData(const XOnlyPubKey& output_key, TaprootSpendData& spenddata) const override;
    bool GetTaprootBuilder(const XOnlyPubKey& output_key, TaprootBuilder& builder) const override;
    FlatSigningProvider& Merge(FlatSigningProvider&& b) LIFETIMEBOUND;
};
class FillableSigningProvider : public SigningProvider
{
protected:
    using KeyMap = std::map<CKeyID, CKey>;
    using ScriptMap = std::map<CScriptID, CScript>;
    KeyMap mapKeys GUARDED_BY(cs_KeyStore);
    ScriptMap mapScripts GUARDED_BY(cs_KeyStore);
    void ImplicitlyLearnRelatedKeyScripts(const CPubKey& pubkey) EXCLUSIVE_LOCKS_REQUIRED(cs_KeyStore);
public:
    mutable RecursiveMutex cs_KeyStore;
    virtual bool AddKeyPubKey(const CKey& key, const CPubKey &pubkey);
    virtual bool AddKey(const CKey &key) { return AddKeyPubKey(key, key.GetPubKey()); }
    virtual bool GetPubKey(const CKeyID &address, CPubKey& vchPubKeyOut) const override;
    virtual bool HaveKey(const CKeyID &address) const override;
    virtual std::set<CKeyID> GetKeys() const;
    virtual bool GetKey(const CKeyID &address, CKey &keyOut) const override;
    virtual bool AddCScript(const CScript& redeemScript);
    virtual bool HaveCScript(const CScriptID &hash) const override;
    virtual std::set<CScriptID> GetCScripts() const;
    virtual bool GetCScript(const CScriptID &hash, CScript& redeemScriptOut) const override;
};
CKeyID GetKeyForDestination(const SigningProvider& store, const CTxDestination& dest);
#endif
