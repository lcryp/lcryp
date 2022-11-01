#ifndef LCRYP_WALLET_SCRIPTPUBKEYMAN_H
#define LCRYP_WALLET_SCRIPTPUBKEYMAN_H
#include <psbt.h>
#include <script/descriptor.h>
#include <script/signingprovider.h>
#include <script/standard.h>
#include <util/error.h>
#include <util/message.h>
#include <util/result.h>
#include <util/time.h>
#include <wallet/crypter.h>
#include <wallet/ismine.h>
#include <wallet/walletdb.h>
#include <wallet/walletutil.h>
#include <boost/signals2/signal.hpp>
#include <optional>
#include <unordered_map>
enum class OutputType;
struct bilingual_str;
namespace wallet {
class WalletStorage
{
public:
    virtual ~WalletStorage() = default;
    virtual const std::string GetDisplayName() const = 0;
    virtual WalletDatabase& GetDatabase() const = 0;
    virtual bool IsWalletFlagSet(uint64_t) const = 0;
    virtual void UnsetBlankWalletFlag(WalletBatch&) = 0;
    virtual bool CanSupportFeature(enum WalletFeature) const = 0;
    virtual void SetMinVersion(enum WalletFeature, WalletBatch* = nullptr) = 0;
    virtual const CKeyingMaterial& GetEncryptionKey() const = 0;
    virtual bool HasEncryptionKeys() const = 0;
    virtual bool IsLocked() const = 0;
};
static const unsigned int DEFAULT_KEYPOOL_SIZE = 1000;
std::vector<CKeyID> GetAffectedKeys(const CScript& spk, const SigningProvider& provider);
class CKeyPool
{
public:
    int64_t nTime;
    CPubKey vchPubKey;
    bool fInternal;
    bool m_pre_split;
    CKeyPool();
    CKeyPool(const CPubKey& vchPubKeyIn, bool internalIn);
    template<typename Stream>
    void Serialize(Stream& s) const
    {
        int nVersion = s.GetVersion();
        if (!(s.GetType() & SER_GETHASH)) {
            s << nVersion;
        }
        s << nTime << vchPubKey << fInternal << m_pre_split;
    }
    template<typename Stream>
    void Unserialize(Stream& s)
    {
        int nVersion = s.GetVersion();
        if (!(s.GetType() & SER_GETHASH)) {
            s >> nVersion;
        }
        s >> nTime >> vchPubKey;
        try {
            s >> fInternal;
        } catch (std::ios_base::failure&) {
            fInternal = false;
        }
        try {
            s >> m_pre_split;
        } catch (std::ios_base::failure&) {
            m_pre_split = false;
        }
    }
};
struct WalletDestination
{
    CTxDestination dest;
    std::optional<bool> internal;
};
class ScriptPubKeyMan
{
protected:
    WalletStorage& m_storage;
public:
    explicit ScriptPubKeyMan(WalletStorage& storage) : m_storage(storage) {}
    virtual ~ScriptPubKeyMan() {};
    virtual util::Result<CTxDestination> GetNewDestination(const OutputType type) { return util::Error{Untranslated("Not supported")}; }
    virtual isminetype IsMine(const CScript& script) const { return ISMINE_NO; }
    virtual bool CheckDecryptionKey(const CKeyingMaterial& master_key, bool accept_no_keys = false) { return false; }
    virtual bool Encrypt(const CKeyingMaterial& master_key, WalletBatch* batch) { return false; }
    virtual util::Result<CTxDestination> GetReservedDestination(const OutputType type, bool internal, int64_t& index, CKeyPool& keypool) { return util::Error{Untranslated("Not supported")}; }
    virtual void KeepDestination(int64_t index, const OutputType& type) {}
    virtual void ReturnDestination(int64_t index, bool internal, const CTxDestination& addr) {}
    virtual bool TopUp(unsigned int size = 0) { return false; }
    virtual std::vector<WalletDestination> MarkUnusedAddresses(const CScript& script) { return {}; }
    virtual bool SetupGeneration(bool force = false) { return false; }
    virtual bool IsHDEnabled() const { return false; }
    virtual bool CanGetAddresses(bool internal = false) const { return false; }
    virtual bool Upgrade(int prev_version, int new_version, bilingual_str& error) { return true; }
    virtual bool HavePrivateKeys() const { return false; }
    virtual void RewriteDB() {}
    virtual std::optional<int64_t> GetOldestKeyPoolTime() const { return GetTime(); }
    virtual unsigned int GetKeyPoolSize() const { return 0; }
    virtual int64_t GetTimeFirstKey() const { return 0; }
    virtual std::unique_ptr<CKeyMetadata> GetMetadata(const CTxDestination& dest) const { return nullptr; }
    virtual std::unique_ptr<SigningProvider> GetSolvingProvider(const CScript& script) const { return nullptr; }
    virtual bool CanProvide(const CScript& script, SignatureData& sigdata) { return false; }
    virtual bool SignTransaction(CMutableTransaction& tx, const std::map<COutPoint, Coin>& coins, int sighash, std::map<int, bilingual_str>& input_errors) const { return false; }
    virtual SigningResult SignMessage(const std::string& message, const PKHash& pkhash, std::string& str_sig) const { return SigningResult::SIGNING_FAILED; };
    virtual TransactionError FillPSBT(PartiallySignedTransaction& psbt, const PrecomputedTransactionData& txdata, int sighash_type = SIGHASH_DEFAULT, bool sign = true, bool bip32derivs = false, int* n_signed = nullptr, bool finalize = true) const { return TransactionError::INVALID_PSBT; }
    virtual uint256 GetID() const { return uint256(); }
    virtual const std::unordered_set<CScript, SaltedSipHasher> GetScriptPubKeys() const { return {}; };
    template<typename... Params>
    void WalletLogPrintf(std::string fmt, Params... parameters) const {
        LogPrintf(("%s " + fmt).c_str(), m_storage.GetDisplayName(), parameters...);
    };
    boost::signals2::signal<void (bool fHaveWatchOnly)> NotifyWatchonlyChanged;
    boost::signals2::signal<void ()> NotifyCanGetAddressesChanged;
};
static const std::unordered_set<OutputType> LEGACY_OUTPUT_TYPES {
    OutputType::LEGACY,
    OutputType::P2SH_SEGWIT,
    OutputType::BECH32,
};
class DescriptorScriptPubKeyMan;
class LegacyScriptPubKeyMan : public ScriptPubKeyMan, public FillableSigningProvider
{
private:
    bool fDecryptionThoroughlyChecked = true;
    using WatchOnlySet = std::set<CScript>;
    using WatchKeyMap = std::map<CKeyID, CPubKey>;
    WalletBatch *encrypted_batch GUARDED_BY(cs_KeyStore) = nullptr;
    using CryptedKeyMap = std::map<CKeyID, std::pair<CPubKey, std::vector<unsigned char>>>;
    CryptedKeyMap mapCryptedKeys GUARDED_BY(cs_KeyStore);
    WatchOnlySet setWatchOnly GUARDED_BY(cs_KeyStore);
    WatchKeyMap mapWatchKeys GUARDED_BY(cs_KeyStore);
    int64_t nTimeFirstKey GUARDED_BY(cs_KeyStore) = 0;
    bool AddKeyPubKeyInner(const CKey& key, const CPubKey &pubkey);
    bool AddCryptedKeyInner(const CPubKey &vchPubKey, const std::vector<unsigned char> &vchCryptedSecret);
    bool AddWatchOnly(const CScript& dest) EXCLUSIVE_LOCKS_REQUIRED(cs_KeyStore);
    bool AddWatchOnlyWithDB(WalletBatch &batch, const CScript& dest) EXCLUSIVE_LOCKS_REQUIRED(cs_KeyStore);
    bool AddWatchOnlyInMem(const CScript &dest);
    bool AddWatchOnlyWithDB(WalletBatch &batch, const CScript& dest, int64_t create_time) EXCLUSIVE_LOCKS_REQUIRED(cs_KeyStore);
    bool AddKeyPubKeyWithDB(WalletBatch &batch,const CKey& key, const CPubKey &pubkey) EXCLUSIVE_LOCKS_REQUIRED(cs_KeyStore);
    void AddKeypoolPubkeyWithDB(const CPubKey& pubkey, const bool internal, WalletBatch& batch);
    bool AddCScriptWithDB(WalletBatch& batch, const CScript& script);
    bool AddKeyOriginWithDB(WalletBatch& batch, const CPubKey& pubkey, const KeyOriginInfo& info);
    CHDChain m_hd_chain;
    std::unordered_map<CKeyID, CHDChain, SaltedSipHasher> m_inactive_hd_chains;
    void DeriveNewChildKey(WalletBatch& batch, CKeyMetadata& metadata, CKey& secret, CHDChain& hd_chain, bool internal = false) EXCLUSIVE_LOCKS_REQUIRED(cs_KeyStore);
    std::set<int64_t> setInternalKeyPool GUARDED_BY(cs_KeyStore);
    std::set<int64_t> setExternalKeyPool GUARDED_BY(cs_KeyStore);
    std::set<int64_t> set_pre_split_keypool GUARDED_BY(cs_KeyStore);
    int64_t m_max_keypool_index GUARDED_BY(cs_KeyStore) = 0;
    std::map<CKeyID, int64_t> m_pool_key_to_index;
    std::map<int64_t, CKeyID> m_index_to_reserved_key;
    bool GetKeyFromPool(CPubKey &key, const OutputType type, bool internal = false);
    bool ReserveKeyFromKeyPool(int64_t& nIndex, CKeyPool& keypool, bool fRequestedInternal);
    bool TopUpInactiveHDChain(const CKeyID seed_id, int64_t index, bool internal);
    bool TopUpChain(CHDChain& chain, unsigned int size);
public:
    using ScriptPubKeyMan::ScriptPubKeyMan;
    util::Result<CTxDestination> GetNewDestination(const OutputType type) override;
    isminetype IsMine(const CScript& script) const override;
    bool CheckDecryptionKey(const CKeyingMaterial& master_key, bool accept_no_keys = false) override;
    bool Encrypt(const CKeyingMaterial& master_key, WalletBatch* batch) override;
    util::Result<CTxDestination> GetReservedDestination(const OutputType type, bool internal, int64_t& index, CKeyPool& keypool) override;
    void KeepDestination(int64_t index, const OutputType& type) override;
    void ReturnDestination(int64_t index, bool internal, const CTxDestination&) override;
    bool TopUp(unsigned int size = 0) override;
    std::vector<WalletDestination> MarkUnusedAddresses(const CScript& script) override;
    void UpgradeKeyMetadata();
    bool IsHDEnabled() const override;
    bool SetupGeneration(bool force = false) override;
    bool Upgrade(int prev_version, int new_version, bilingual_str& error) override;
    bool HavePrivateKeys() const override;
    void RewriteDB() override;
    std::optional<int64_t> GetOldestKeyPoolTime() const override;
    size_t KeypoolCountExternalKeys() const;
    unsigned int GetKeyPoolSize() const override;
    int64_t GetTimeFirstKey() const override;
    std::unique_ptr<CKeyMetadata> GetMetadata(const CTxDestination& dest) const override;
    bool CanGetAddresses(bool internal = false) const override;
    std::unique_ptr<SigningProvider> GetSolvingProvider(const CScript& script) const override;
    bool CanProvide(const CScript& script, SignatureData& sigdata) override;
    bool SignTransaction(CMutableTransaction& tx, const std::map<COutPoint, Coin>& coins, int sighash, std::map<int, bilingual_str>& input_errors) const override;
    SigningResult SignMessage(const std::string& message, const PKHash& pkhash, std::string& str_sig) const override;
    TransactionError FillPSBT(PartiallySignedTransaction& psbt, const PrecomputedTransactionData& txdata, int sighash_type = SIGHASH_DEFAULT, bool sign = true, bool bip32derivs = false, int* n_signed = nullptr, bool finalize = true) const override;
    uint256 GetID() const override;
    std::map<CKeyID, CKeyMetadata> mapKeyMetadata GUARDED_BY(cs_KeyStore);
    std::map<CScriptID, CKeyMetadata> m_script_metadata GUARDED_BY(cs_KeyStore);
    bool AddKeyPubKey(const CKey& key, const CPubKey &pubkey) override;
    bool LoadKey(const CKey& key, const CPubKey &pubkey);
    bool AddCryptedKey(const CPubKey &vchPubKey, const std::vector<unsigned char> &vchCryptedSecret);
    bool LoadCryptedKey(const CPubKey &vchPubKey, const std::vector<unsigned char> &vchCryptedSecret, bool checksum_valid);
    void UpdateTimeFirstKey(int64_t nCreateTime) EXCLUSIVE_LOCKS_REQUIRED(cs_KeyStore);
    bool LoadCScript(const CScript& redeemScript);
    void LoadKeyMetadata(const CKeyID& keyID, const CKeyMetadata &metadata);
    void LoadScriptMetadata(const CScriptID& script_id, const CKeyMetadata &metadata);
    CPubKey GenerateNewKey(WalletBatch& batch, CHDChain& hd_chain, bool internal = false) EXCLUSIVE_LOCKS_REQUIRED(cs_KeyStore);
    void AddHDChain(const CHDChain& chain);
    void LoadHDChain(const CHDChain& chain);
    const CHDChain& GetHDChain() const { return m_hd_chain; }
    void AddInactiveHDChain(const CHDChain& chain);
    bool LoadWatchOnly(const CScript &dest);
    bool HaveWatchOnly(const CScript &dest) const;
    bool HaveWatchOnly() const;
    bool RemoveWatchOnly(const CScript &dest);
    bool AddWatchOnly(const CScript& dest, int64_t nCreateTime) EXCLUSIVE_LOCKS_REQUIRED(cs_KeyStore);
    bool GetWatchPubKey(const CKeyID &address, CPubKey &pubkey_out) const;
    bool HaveKey(const CKeyID &address) const override;
    bool GetKey(const CKeyID &address, CKey& keyOut) const override;
    bool GetPubKey(const CKeyID &address, CPubKey& vchPubKeyOut) const override;
    bool AddCScript(const CScript& redeemScript) override;
    bool GetKeyOrigin(const CKeyID& keyid, KeyOriginInfo& info) const override;
    void LoadKeyPool(int64_t nIndex, const CKeyPool &keypool);
    bool NewKeyPool();
    void MarkPreSplitKeys() EXCLUSIVE_LOCKS_REQUIRED(cs_KeyStore);
    bool ImportScripts(const std::set<CScript> scripts, int64_t timestamp) EXCLUSIVE_LOCKS_REQUIRED(cs_KeyStore);
    bool ImportPrivKeys(const std::map<CKeyID, CKey>& privkey_map, const int64_t timestamp) EXCLUSIVE_LOCKS_REQUIRED(cs_KeyStore);
    bool ImportPubKeys(const std::vector<CKeyID>& ordered_pubkeys, const std::map<CKeyID, CPubKey>& pubkey_map, const std::map<CKeyID, std::pair<CPubKey, KeyOriginInfo>>& key_origins, const bool add_keypool, const bool internal, const int64_t timestamp) EXCLUSIVE_LOCKS_REQUIRED(cs_KeyStore);
    bool ImportScriptPubKeys(const std::set<CScript>& script_pub_keys, const bool have_solving_data, const int64_t timestamp) EXCLUSIVE_LOCKS_REQUIRED(cs_KeyStore);
    bool CanGenerateKeys() const;
    CPubKey GenerateNewSeed();
    CPubKey DeriveNewSeed(const CKey& key);
    void SetHDSeed(const CPubKey& key);
    void LearnRelatedScripts(const CPubKey& key, OutputType);
    void LearnAllRelatedScripts(const CPubKey& key);
    std::vector<CKeyPool> MarkReserveKeysAsUsed(int64_t keypool_id) EXCLUSIVE_LOCKS_REQUIRED(cs_KeyStore);
    const std::map<CKeyID, int64_t>& GetAllReserveKeys() const { return m_pool_key_to_index; }
    std::set<CKeyID> GetKeys() const override;
    const std::unordered_set<CScript, SaltedSipHasher> GetScriptPubKeys() const override;
    std::optional<MigrationData> MigrateToDescriptor();
    bool DeleteRecords();
};
class LegacySigningProvider : public SigningProvider
{
private:
    const LegacyScriptPubKeyMan& m_spk_man;
public:
    explicit LegacySigningProvider(const LegacyScriptPubKeyMan& spk_man) : m_spk_man(spk_man) {}
    bool GetCScript(const CScriptID &scriptid, CScript& script) const override { return m_spk_man.GetCScript(scriptid, script); }
    bool HaveCScript(const CScriptID &scriptid) const override { return m_spk_man.HaveCScript(scriptid); }
    bool GetPubKey(const CKeyID &address, CPubKey& pubkey) const override { return m_spk_man.GetPubKey(address, pubkey); }
    bool GetKey(const CKeyID &address, CKey& key) const override { return false; }
    bool HaveKey(const CKeyID &address) const override { return false; }
    bool GetKeyOrigin(const CKeyID& keyid, KeyOriginInfo& info) const override { return m_spk_man.GetKeyOrigin(keyid, info); }
};
class DescriptorScriptPubKeyMan : public ScriptPubKeyMan
{
private:
    using ScriptPubKeyMap = std::map<CScript, int32_t>;
    using PubKeyMap = std::map<CPubKey, int32_t>;
    using CryptedKeyMap = std::map<CKeyID, std::pair<CPubKey, std::vector<unsigned char>>>;
    using KeyMap = std::map<CKeyID, CKey>;
    ScriptPubKeyMap m_map_script_pub_keys GUARDED_BY(cs_desc_man);
    PubKeyMap m_map_pubkeys GUARDED_BY(cs_desc_man);
    int32_t m_max_cached_index = -1;
    KeyMap m_map_keys GUARDED_BY(cs_desc_man);
    CryptedKeyMap m_map_crypted_keys GUARDED_BY(cs_desc_man);
    bool m_decryption_thoroughly_checked = false;
    bool AddDescriptorKeyWithDB(WalletBatch& batch, const CKey& key, const CPubKey &pubkey) EXCLUSIVE_LOCKS_REQUIRED(cs_desc_man);
    KeyMap GetKeys() const EXCLUSIVE_LOCKS_REQUIRED(cs_desc_man);
    mutable std::map<int32_t, FlatSigningProvider> m_map_signing_providers;
    std::unique_ptr<FlatSigningProvider> GetSigningProvider(const CScript& script, bool include_private = false) const;
    std::unique_ptr<FlatSigningProvider> GetSigningProvider(const CPubKey& pubkey) const;
    std::unique_ptr<FlatSigningProvider> GetSigningProvider(int32_t index, bool include_private = false) const EXCLUSIVE_LOCKS_REQUIRED(cs_desc_man);
protected:
  WalletDescriptor m_wallet_descriptor GUARDED_BY(cs_desc_man);
public:
    DescriptorScriptPubKeyMan(WalletStorage& storage, WalletDescriptor& descriptor)
        :   ScriptPubKeyMan(storage),
            m_wallet_descriptor(descriptor)
        {}
    DescriptorScriptPubKeyMan(WalletStorage& storage)
        :   ScriptPubKeyMan(storage)
        {}
    mutable RecursiveMutex cs_desc_man;
    util::Result<CTxDestination> GetNewDestination(const OutputType type) override;
    isminetype IsMine(const CScript& script) const override;
    bool CheckDecryptionKey(const CKeyingMaterial& master_key, bool accept_no_keys = false) override;
    bool Encrypt(const CKeyingMaterial& master_key, WalletBatch* batch) override;
    util::Result<CTxDestination> GetReservedDestination(const OutputType type, bool internal, int64_t& index, CKeyPool& keypool) override;
    void ReturnDestination(int64_t index, bool internal, const CTxDestination& addr) override;
    bool TopUp(unsigned int size = 0) override;
    std::vector<WalletDestination> MarkUnusedAddresses(const CScript& script) override;
    bool IsHDEnabled() const override;
    bool SetupDescriptorGeneration(const CExtKey& master_key, OutputType addr_type, bool internal);
    bool SetupDescriptor(std::unique_ptr<Descriptor>desc);
    bool HavePrivateKeys() const override;
    std::optional<int64_t> GetOldestKeyPoolTime() const override;
    unsigned int GetKeyPoolSize() const override;
    int64_t GetTimeFirstKey() const override;
    std::unique_ptr<CKeyMetadata> GetMetadata(const CTxDestination& dest) const override;
    bool CanGetAddresses(bool internal = false) const override;
    std::unique_ptr<SigningProvider> GetSolvingProvider(const CScript& script) const override;
    bool CanProvide(const CScript& script, SignatureData& sigdata) override;
    bool SignTransaction(CMutableTransaction& tx, const std::map<COutPoint, Coin>& coins, int sighash, std::map<int, bilingual_str>& input_errors) const override;
    SigningResult SignMessage(const std::string& message, const PKHash& pkhash, std::string& str_sig) const override;
    TransactionError FillPSBT(PartiallySignedTransaction& psbt, const PrecomputedTransactionData& txdata, int sighash_type = SIGHASH_DEFAULT, bool sign = true, bool bip32derivs = false, int* n_signed = nullptr, bool finalize = true) const override;
    uint256 GetID() const override;
    void SetCache(const DescriptorCache& cache);
    bool AddKey(const CKeyID& key_id, const CKey& key);
    bool AddCryptedKey(const CKeyID& key_id, const CPubKey& pubkey, const std::vector<unsigned char>& crypted_key);
    bool HasWalletDescriptor(const WalletDescriptor& desc) const;
    void UpdateWalletDescriptor(WalletDescriptor& descriptor);
    bool CanUpdateToWalletDescriptor(const WalletDescriptor& descriptor, std::string& error);
    void AddDescriptorKey(const CKey& key, const CPubKey &pubkey);
    void WriteDescriptor();
    const WalletDescriptor GetWalletDescriptor() const EXCLUSIVE_LOCKS_REQUIRED(cs_desc_man);
    const std::unordered_set<CScript, SaltedSipHasher> GetScriptPubKeys() const override;
    bool GetDescriptorString(std::string& out, const bool priv) const;
    void UpgradeDescriptorCache();
};
}
#endif
