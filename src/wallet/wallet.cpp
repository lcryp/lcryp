#include <wallet/wallet.h>
#include <chain.h>
#include <consensus/amount.h>
#include <consensus/consensus.h>
#include <consensus/validation.h>
#include <external_signer.h>
#include <fs.h>
#include <interfaces/chain.h>
#include <interfaces/wallet.h>
#include <key.h>
#include <key_io.h>
#include <outputtype.h>
#include <policy/fees.h>
#include <policy/policy.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <psbt.h>
#include <script/descriptor.h>
#include <script/script.h>
#include <script/signingprovider.h>
#include <txmempool.h>
#include <util/bip32.h>
#include <util/check.h>
#include <util/error.h>
#include <util/fees.h>
#include <util/moneystr.h>
#include <util/rbf.h>
#include <util/string.h>
#include <util/translation.h>
#include <wallet/coincontrol.h>
#include <wallet/context.h>
#include <wallet/fees.h>
#include <wallet/external_signer_scriptpubkeyman.h>
#include <univalue.h>
#include <algorithm>
#include <assert.h>
#include <optional>
using interfaces::FoundBlock;
namespace wallet {
const std::map<uint64_t,std::string> WALLET_FLAG_CAVEATS{
    {WALLET_FLAG_AVOID_REUSE,
        "You need to rescan the blockchain in order to correctly mark used "
        "destinations in the past. Until this is done, some destinations may "
        "be considered unused, even if the opposite is the case."
    },
};
bool AddWalletSetting(interfaces::Chain& chain, const std::string& wallet_name)
{
    util::SettingsValue setting_value = chain.getRwSetting("wallet");
    if (!setting_value.isArray()) setting_value.setArray();
    for (const util::SettingsValue& value : setting_value.getValues()) {
        if (value.isStr() && value.get_str() == wallet_name) return true;
    }
    setting_value.push_back(wallet_name);
    return chain.updateRwSetting("wallet", setting_value);
}
bool RemoveWalletSetting(interfaces::Chain& chain, const std::string& wallet_name)
{
    util::SettingsValue setting_value = chain.getRwSetting("wallet");
    if (!setting_value.isArray()) return true;
    util::SettingsValue new_value(util::SettingsValue::VARR);
    for (const util::SettingsValue& value : setting_value.getValues()) {
        if (!value.isStr() || value.get_str() != wallet_name) new_value.push_back(value);
    }
    if (new_value.size() == setting_value.size()) return true;
    return chain.updateRwSetting("wallet", new_value);
}
static void UpdateWalletSetting(interfaces::Chain& chain,
                                const std::string& wallet_name,
                                std::optional<bool> load_on_startup,
                                std::vector<bilingual_str>& warnings)
{
    if (!load_on_startup) return;
    if (load_on_startup.value() && !AddWalletSetting(chain, wallet_name)) {
        warnings.emplace_back(Untranslated("Wallet load on startup setting could not be updated, so wallet may not be loaded next node startup."));
    } else if (!load_on_startup.value() && !RemoveWalletSetting(chain, wallet_name)) {
        warnings.emplace_back(Untranslated("Wallet load on startup setting could not be updated, so wallet may still be loaded next node startup."));
    }
}
static void RefreshMempoolStatus(CWalletTx& tx, interfaces::Chain& chain)
{
    if (chain.isInMempool(tx.GetHash())) {
        tx.m_state = TxStateInMempool();
    } else if (tx.state<TxStateInMempool>()) {
        tx.m_state = TxStateInactive();
    }
}
bool AddWallet(WalletContext& context, const std::shared_ptr<CWallet>& wallet)
{
    LOCK(context.wallets_mutex);
    assert(wallet);
    std::vector<std::shared_ptr<CWallet>>::const_iterator i = std::find(context.wallets.begin(), context.wallets.end(), wallet);
    if (i != context.wallets.end()) return false;
    context.wallets.push_back(wallet);
    wallet->ConnectScriptPubKeyManNotifiers();
    wallet->NotifyCanGetAddressesChanged();
    return true;
}
bool RemoveWallet(WalletContext& context, const std::shared_ptr<CWallet>& wallet, std::optional<bool> load_on_start, std::vector<bilingual_str>& warnings)
{
    assert(wallet);
    interfaces::Chain& chain = wallet->chain();
    std::string name = wallet->GetName();
    wallet->m_chain_notifications_handler.reset();
    LOCK(context.wallets_mutex);
    std::vector<std::shared_ptr<CWallet>>::iterator i = std::find(context.wallets.begin(), context.wallets.end(), wallet);
    if (i == context.wallets.end()) return false;
    context.wallets.erase(i);
    UpdateWalletSetting(chain, name, load_on_start, warnings);
    return true;
}
bool RemoveWallet(WalletContext& context, const std::shared_ptr<CWallet>& wallet, std::optional<bool> load_on_start)
{
    std::vector<bilingual_str> warnings;
    return RemoveWallet(context, wallet, load_on_start, warnings);
}
std::vector<std::shared_ptr<CWallet>> GetWallets(WalletContext& context)
{
    LOCK(context.wallets_mutex);
    return context.wallets;
}
std::shared_ptr<CWallet> GetDefaultWallet(WalletContext& context, size_t& count)
{
    LOCK(context.wallets_mutex);
    count = context.wallets.size();
    return count == 1 ? context.wallets[0] : nullptr;
}
std::shared_ptr<CWallet> GetWallet(WalletContext& context, const std::string& name)
{
    LOCK(context.wallets_mutex);
    for (const std::shared_ptr<CWallet>& wallet : context.wallets) {
        if (wallet->GetName() == name) return wallet;
    }
    return nullptr;
}
std::unique_ptr<interfaces::Handler> HandleLoadWallet(WalletContext& context, LoadWalletFn load_wallet)
{
    LOCK(context.wallets_mutex);
    auto it = context.wallet_load_fns.emplace(context.wallet_load_fns.end(), std::move(load_wallet));
    return interfaces::MakeHandler([&context, it] { LOCK(context.wallets_mutex); context.wallet_load_fns.erase(it); });
}
void NotifyWalletLoaded(WalletContext& context, const std::shared_ptr<CWallet>& wallet)
{
    LOCK(context.wallets_mutex);
    for (auto& load_wallet : context.wallet_load_fns) {
        load_wallet(interfaces::MakeWallet(context, wallet));
    }
}
static GlobalMutex g_loading_wallet_mutex;
static GlobalMutex g_wallet_release_mutex;
static std::condition_variable g_wallet_release_cv;
static std::set<std::string> g_loading_wallet_set GUARDED_BY(g_loading_wallet_mutex);
static std::set<std::string> g_unloading_wallet_set GUARDED_BY(g_wallet_release_mutex);
static void ReleaseWallet(CWallet* wallet)
{
    const std::string name = wallet->GetName();
    wallet->WalletLogPrintf("Releasing wallet\n");
    wallet->Flush();
    delete wallet;
    {
        LOCK(g_wallet_release_mutex);
        if (g_unloading_wallet_set.erase(name) == 0) {
            return;
        }
    }
    g_wallet_release_cv.notify_all();
}
void UnloadWallet(std::shared_ptr<CWallet>&& wallet)
{
    const std::string name = wallet->GetName();
    {
        LOCK(g_wallet_release_mutex);
        auto it = g_unloading_wallet_set.insert(name);
        assert(it.second);
    }
    wallet->NotifyUnload();
    wallet.reset();
    {
        WAIT_LOCK(g_wallet_release_mutex, lock);
        while (g_unloading_wallet_set.count(name) == 1) {
            g_wallet_release_cv.wait(lock);
        }
    }
}
namespace {
std::shared_ptr<CWallet> LoadWalletInternal(WalletContext& context, const std::string& name, std::optional<bool> load_on_start, const DatabaseOptions& options, DatabaseStatus& status, bilingual_str& error, std::vector<bilingual_str>& warnings)
{
    try {
        std::unique_ptr<WalletDatabase> database = MakeWalletDatabase(name, options, status, error);
        if (!database) {
            error = Untranslated("Wallet file verification failed.") + Untranslated(" ") + error;
            return nullptr;
        }
        context.chain->initMessage(_("Loading wallet…").translated);
        const std::shared_ptr<CWallet> wallet = CWallet::Create(context, name, std::move(database), options.create_flags, error, warnings);
        if (!wallet) {
            error = Untranslated("Wallet loading failed.") + Untranslated(" ") + error;
            status = DatabaseStatus::FAILED_LOAD;
            return nullptr;
        }
        NotifyWalletLoaded(context, wallet);
        AddWallet(context, wallet);
        wallet->postInitProcess();
        UpdateWalletSetting(*context.chain, name, load_on_start, warnings);
        return wallet;
    } catch (const std::runtime_error& e) {
        error = Untranslated(e.what());
        status = DatabaseStatus::FAILED_LOAD;
        return nullptr;
    }
}
}
std::shared_ptr<CWallet> LoadWallet(WalletContext& context, const std::string& name, std::optional<bool> load_on_start, const DatabaseOptions& options, DatabaseStatus& status, bilingual_str& error, std::vector<bilingual_str>& warnings)
{
    auto result = WITH_LOCK(g_loading_wallet_mutex, return g_loading_wallet_set.insert(name));
    if (!result.second) {
        error = Untranslated("Wallet already loading.");
        status = DatabaseStatus::FAILED_LOAD;
        return nullptr;
    }
    auto wallet = LoadWalletInternal(context, name, load_on_start, options, status, error, warnings);
    WITH_LOCK(g_loading_wallet_mutex, g_loading_wallet_set.erase(result.first));
    return wallet;
}
std::shared_ptr<CWallet> CreateWallet(WalletContext& context, const std::string& name, std::optional<bool> load_on_start, DatabaseOptions& options, DatabaseStatus& status, bilingual_str& error, std::vector<bilingual_str>& warnings)
{
    uint64_t wallet_creation_flags = options.create_flags;
    const SecureString& passphrase = options.create_passphrase;
    if (wallet_creation_flags & WALLET_FLAG_DESCRIPTORS) options.require_format = DatabaseFormat::SQLITE;
    bool create_blank = (wallet_creation_flags & WALLET_FLAG_BLANK_WALLET);
    if (!passphrase.empty()) {
        wallet_creation_flags |= WALLET_FLAG_BLANK_WALLET;
    }
    if ((wallet_creation_flags & WALLET_FLAG_EXTERNAL_SIGNER) && !(wallet_creation_flags & WALLET_FLAG_DISABLE_PRIVATE_KEYS)) {
        error = Untranslated("Private keys must be disabled when using an external signer");
        status = DatabaseStatus::FAILED_CREATE;
        return nullptr;
    }
    if ((wallet_creation_flags & WALLET_FLAG_EXTERNAL_SIGNER) && !(wallet_creation_flags & WALLET_FLAG_DESCRIPTORS)) {
        error = Untranslated("Descriptor support must be enabled when using an external signer");
        status = DatabaseStatus::FAILED_CREATE;
        return nullptr;
    }
    if (!passphrase.empty() && (wallet_creation_flags & WALLET_FLAG_DISABLE_PRIVATE_KEYS)) {
        error = Untranslated("Passphrase provided but private keys are disabled. A passphrase is only used to encrypt private keys, so cannot be used for wallets with private keys disabled.");
        status = DatabaseStatus::FAILED_CREATE;
        return nullptr;
    }
    std::unique_ptr<WalletDatabase> database = MakeWalletDatabase(name, options, status, error);
    if (!database) {
        error = Untranslated("Wallet file verification failed.") + Untranslated(" ") + error;
        status = DatabaseStatus::FAILED_VERIFY;
        return nullptr;
    }
    context.chain->initMessage(_("Loading wallet…").translated);
    const std::shared_ptr<CWallet> wallet = CWallet::Create(context, name, std::move(database), wallet_creation_flags, error, warnings);
    if (!wallet) {
        error = Untranslated("Wallet creation failed.") + Untranslated(" ") + error;
        status = DatabaseStatus::FAILED_CREATE;
        return nullptr;
    }
    if (!passphrase.empty() && !(wallet_creation_flags & WALLET_FLAG_DISABLE_PRIVATE_KEYS)) {
        if (!wallet->EncryptWallet(passphrase)) {
            error = Untranslated("Error: Wallet created but failed to encrypt.");
            status = DatabaseStatus::FAILED_ENCRYPT;
            return nullptr;
        }
        if (!create_blank) {
            if (!wallet->Unlock(passphrase)) {
                error = Untranslated("Error: Wallet was encrypted but could not be unlocked");
                status = DatabaseStatus::FAILED_ENCRYPT;
                return nullptr;
            }
            {
                LOCK(wallet->cs_wallet);
                if (wallet->IsWalletFlagSet(WALLET_FLAG_DESCRIPTORS)) {
                    wallet->SetupDescriptorScriptPubKeyMans();
                } else {
                    for (auto spk_man : wallet->GetActiveScriptPubKeyMans()) {
                        if (!spk_man->SetupGeneration()) {
                            error = Untranslated("Unable to generate initial keys");
                            status = DatabaseStatus::FAILED_CREATE;
                            return nullptr;
                        }
                    }
                }
            }
            wallet->Lock();
        }
    }
    NotifyWalletLoaded(context, wallet);
    AddWallet(context, wallet);
    wallet->postInitProcess();
    UpdateWalletSetting(*context.chain, name, load_on_start, warnings);
    if (!(wallet_creation_flags & WALLET_FLAG_DESCRIPTORS)) {
        warnings.push_back(_("Wallet created successfully. The legacy wallet type is being deprecated and support for creating and opening legacy wallets will be removed in the future."));
    }
    status = DatabaseStatus::SUCCESS;
    return wallet;
}
std::shared_ptr<CWallet> RestoreWallet(WalletContext& context, const fs::path& backup_file, const std::string& wallet_name, std::optional<bool> load_on_start, DatabaseStatus& status, bilingual_str& error, std::vector<bilingual_str>& warnings)
{
    DatabaseOptions options;
    ReadDatabaseArgs(*context.args, options);
    options.require_existing = true;
    const fs::path wallet_path = fsbridge::AbsPathJoin(GetWalletDir(), fs::u8path(wallet_name));
    auto wallet_file = wallet_path / "wallet.dat";
    std::shared_ptr<CWallet> wallet;
    try {
        if (!fs::exists(backup_file)) {
            error = Untranslated("Backup file does not exist");
            status = DatabaseStatus::FAILED_INVALID_BACKUP_FILE;
            return nullptr;
        }
        if (fs::exists(wallet_path) || !TryCreateDirectories(wallet_path)) {
            error = Untranslated(strprintf("Failed to create database path '%s'. Database already exists.", fs::PathToString(wallet_path)));
            status = DatabaseStatus::FAILED_ALREADY_EXISTS;
            return nullptr;
        }
        fs::copy_file(backup_file, wallet_file, fs::copy_options::none);
        wallet = LoadWallet(context, wallet_name, load_on_start, options, status, error, warnings);
    } catch (const std::exception& e) {
        assert(!wallet);
        if (!error.empty()) error += Untranslated("\n");
        error += strprintf(Untranslated("Unexpected exception: %s"), e.what());
    }
    if (!wallet) {
        fs::remove(wallet_file);
        fs::remove(wallet_path);
    }
    return wallet;
}
const CWalletTx* CWallet::GetWalletTx(const uint256& hash) const
{
    AssertLockHeld(cs_wallet);
    const auto it = mapWallet.find(hash);
    if (it == mapWallet.end())
        return nullptr;
    return &(it->second);
}
void CWallet::UpgradeKeyMetadata()
{
    if (IsLocked() || IsWalletFlagSet(WALLET_FLAG_KEY_ORIGIN_METADATA)) {
        return;
    }
    auto spk_man = GetLegacyScriptPubKeyMan();
    if (!spk_man) {
        return;
    }
    spk_man->UpgradeKeyMetadata();
    SetWalletFlag(WALLET_FLAG_KEY_ORIGIN_METADATA);
}
void CWallet::UpgradeDescriptorCache()
{
    if (!IsWalletFlagSet(WALLET_FLAG_DESCRIPTORS) || IsLocked() || IsWalletFlagSet(WALLET_FLAG_LAST_HARDENED_XPUB_CACHED)) {
        return;
    }
    for (ScriptPubKeyMan* spkm : GetAllScriptPubKeyMans()) {
        DescriptorScriptPubKeyMan* desc_spkm = dynamic_cast<DescriptorScriptPubKeyMan*>(spkm);
        desc_spkm->UpgradeDescriptorCache();
    }
    SetWalletFlag(WALLET_FLAG_LAST_HARDENED_XPUB_CACHED);
}
bool CWallet::Unlock(const SecureString& strWalletPassphrase, bool accept_no_keys)
{
    CCrypter crypter;
    CKeyingMaterial _vMasterKey;
    {
        LOCK(cs_wallet);
        for (const MasterKeyMap::value_type& pMasterKey : mapMasterKeys)
        {
            if(!crypter.SetKeyFromPassphrase(strWalletPassphrase, pMasterKey.second.vchSalt, pMasterKey.second.nDeriveIterations, pMasterKey.second.nDerivationMethod))
                return false;
            if (!crypter.Decrypt(pMasterKey.second.vchCryptedKey, _vMasterKey))
                continue;
            if (Unlock(_vMasterKey, accept_no_keys)) {
                UpgradeKeyMetadata();
                UpgradeDescriptorCache();
                return true;
            }
        }
    }
    return false;
}
bool CWallet::ChangeWalletPassphrase(const SecureString& strOldWalletPassphrase, const SecureString& strNewWalletPassphrase)
{
    bool fWasLocked = IsLocked();
    {
        LOCK(cs_wallet);
        Lock();
        CCrypter crypter;
        CKeyingMaterial _vMasterKey;
        for (MasterKeyMap::value_type& pMasterKey : mapMasterKeys)
        {
            if(!crypter.SetKeyFromPassphrase(strOldWalletPassphrase, pMasterKey.second.vchSalt, pMasterKey.second.nDeriveIterations, pMasterKey.second.nDerivationMethod))
                return false;
            if (!crypter.Decrypt(pMasterKey.second.vchCryptedKey, _vMasterKey))
                return false;
            if (Unlock(_vMasterKey))
            {
                int64_t nStartTime = GetTimeMillis();
                crypter.SetKeyFromPassphrase(strNewWalletPassphrase, pMasterKey.second.vchSalt, pMasterKey.second.nDeriveIterations, pMasterKey.second.nDerivationMethod);
                pMasterKey.second.nDeriveIterations = static_cast<unsigned int>(pMasterKey.second.nDeriveIterations * (100 / ((double)(GetTimeMillis() - nStartTime))));
                nStartTime = GetTimeMillis();
                crypter.SetKeyFromPassphrase(strNewWalletPassphrase, pMasterKey.second.vchSalt, pMasterKey.second.nDeriveIterations, pMasterKey.second.nDerivationMethod);
                pMasterKey.second.nDeriveIterations = (pMasterKey.second.nDeriveIterations + static_cast<unsigned int>(pMasterKey.second.nDeriveIterations * 100 / ((double)(GetTimeMillis() - nStartTime)))) / 2;
                if (pMasterKey.second.nDeriveIterations < 25000)
                    pMasterKey.second.nDeriveIterations = 25000;
                WalletLogPrintf("Wallet passphrase changed to an nDeriveIterations of %i\n", pMasterKey.second.nDeriveIterations);
                if (!crypter.SetKeyFromPassphrase(strNewWalletPassphrase, pMasterKey.second.vchSalt, pMasterKey.second.nDeriveIterations, pMasterKey.second.nDerivationMethod))
                    return false;
                if (!crypter.Encrypt(_vMasterKey, pMasterKey.second.vchCryptedKey))
                    return false;
                WalletBatch(GetDatabase()).WriteMasterKey(pMasterKey.first, pMasterKey.second);
                if (fWasLocked)
                    Lock();
                return true;
            }
        }
    }
    return false;
}
void CWallet::chainStateFlushed(const CBlockLocator& loc)
{
    if (m_attaching_chain) {
        return;
    }
    WalletBatch batch(GetDatabase());
    batch.WriteBestBlock(loc);
}
void CWallet::SetMinVersion(enum WalletFeature nVersion, WalletBatch* batch_in)
{
    LOCK(cs_wallet);
    if (nWalletVersion >= nVersion)
        return;
    WalletLogPrintf("Setting minversion to %d\n", nVersion);
    nWalletVersion = nVersion;
    {
        WalletBatch* batch = batch_in ? batch_in : new WalletBatch(GetDatabase());
        if (nWalletVersion > 40000)
            batch->WriteMinVersion(nWalletVersion);
        if (!batch_in)
            delete batch;
    }
}
std::set<uint256> CWallet::GetConflicts(const uint256& txid) const
{
    std::set<uint256> result;
    AssertLockHeld(cs_wallet);
    const auto it = mapWallet.find(txid);
    if (it == mapWallet.end())
        return result;
    const CWalletTx& wtx = it->second;
    std::pair<TxSpends::const_iterator, TxSpends::const_iterator> range;
    for (const CTxIn& txin : wtx.tx->vin)
    {
        if (mapTxSpends.count(txin.prevout) <= 1)
            continue;
        range = mapTxSpends.equal_range(txin.prevout);
        for (TxSpends::const_iterator _it = range.first; _it != range.second; ++_it)
            result.insert(_it->second);
    }
    return result;
}
bool CWallet::HasWalletSpend(const CTransactionRef& tx) const
{
    AssertLockHeld(cs_wallet);
    const uint256& txid = tx->GetHash();
    for (unsigned int i = 0; i < tx->vout.size(); ++i) {
        auto iter = mapTxSpends.find(COutPoint(txid, i));
        if (iter != mapTxSpends.end()) {
            return true;
        }
    }
    return false;
}
void CWallet::Flush()
{
    GetDatabase().Flush();
}
void CWallet::Close()
{
    GetDatabase().Close();
}
void CWallet::SyncMetaData(std::pair<TxSpends::iterator, TxSpends::iterator> range)
{
    int nMinOrderPos = std::numeric_limits<int>::max();
    const CWalletTx* copyFrom = nullptr;
    for (TxSpends::iterator it = range.first; it != range.second; ++it) {
        const CWalletTx* wtx = &mapWallet.at(it->second);
        if (wtx->nOrderPos < nMinOrderPos) {
            nMinOrderPos = wtx->nOrderPos;
            copyFrom = wtx;
        }
    }
    if (!copyFrom) {
        return;
    }
    for (TxSpends::iterator it = range.first; it != range.second; ++it)
    {
        const uint256& hash = it->second;
        CWalletTx* copyTo = &mapWallet.at(hash);
        if (copyFrom == copyTo) continue;
        assert(copyFrom && "Oldest wallet transaction in range assumed to have been found.");
        if (!copyFrom->IsEquivalentTo(*copyTo)) continue;
        copyTo->mapValue = copyFrom->mapValue;
        copyTo->vOrderForm = copyFrom->vOrderForm;
        copyTo->nTimeSmart = copyFrom->nTimeSmart;
        copyTo->fFromMe = copyFrom->fFromMe;
    }
}
bool CWallet::IsSpent(const COutPoint& outpoint) const
{
    std::pair<TxSpends::const_iterator, TxSpends::const_iterator> range;
    range = mapTxSpends.equal_range(outpoint);
    for (TxSpends::const_iterator it = range.first; it != range.second; ++it) {
        const uint256& wtxid = it->second;
        const auto mit = mapWallet.find(wtxid);
        if (mit != mapWallet.end()) {
            int depth = GetTxDepthInMainChain(mit->second);
            if (depth > 0  || (depth == 0 && !mit->second.isAbandoned()))
                return true;
        }
    }
    return false;
}
void CWallet::AddToSpends(const COutPoint& outpoint, const uint256& wtxid, WalletBatch* batch)
{
    mapTxSpends.insert(std::make_pair(outpoint, wtxid));
    if (batch) {
        UnlockCoin(outpoint, batch);
    } else {
        WalletBatch temp_batch(GetDatabase());
        UnlockCoin(outpoint, &temp_batch);
    }
    std::pair<TxSpends::iterator, TxSpends::iterator> range;
    range = mapTxSpends.equal_range(outpoint);
    SyncMetaData(range);
}
void CWallet::AddToSpends(const CWalletTx& wtx, WalletBatch* batch)
{
    if (wtx.IsCoinBase())
        return;
    for (const CTxIn& txin : wtx.tx->vin)
        AddToSpends(txin.prevout, wtx.GetHash(), batch);
}
bool CWallet::EncryptWallet(const SecureString& strWalletPassphrase)
{
    if (IsCrypted())
        return false;
    CKeyingMaterial _vMasterKey;
    _vMasterKey.resize(WALLET_CRYPTO_KEY_SIZE);
    GetStrongRandBytes(_vMasterKey);
    CMasterKey kMasterKey;
    kMasterKey.vchSalt.resize(WALLET_CRYPTO_SALT_SIZE);
    GetStrongRandBytes(kMasterKey.vchSalt);
    CCrypter crypter;
    int64_t nStartTime = GetTimeMillis();
    crypter.SetKeyFromPassphrase(strWalletPassphrase, kMasterKey.vchSalt, 25000, kMasterKey.nDerivationMethod);
    kMasterKey.nDeriveIterations = static_cast<unsigned int>(2500000 / ((double)(GetTimeMillis() - nStartTime)));
    nStartTime = GetTimeMillis();
    crypter.SetKeyFromPassphrase(strWalletPassphrase, kMasterKey.vchSalt, kMasterKey.nDeriveIterations, kMasterKey.nDerivationMethod);
    kMasterKey.nDeriveIterations = (kMasterKey.nDeriveIterations + static_cast<unsigned int>(kMasterKey.nDeriveIterations * 100 / ((double)(GetTimeMillis() - nStartTime)))) / 2;
    if (kMasterKey.nDeriveIterations < 25000)
        kMasterKey.nDeriveIterations = 25000;
    WalletLogPrintf("Encrypting Wallet with an nDeriveIterations of %i\n", kMasterKey.nDeriveIterations);
    if (!crypter.SetKeyFromPassphrase(strWalletPassphrase, kMasterKey.vchSalt, kMasterKey.nDeriveIterations, kMasterKey.nDerivationMethod))
        return false;
    if (!crypter.Encrypt(_vMasterKey, kMasterKey.vchCryptedKey))
        return false;
    {
        LOCK(cs_wallet);
        mapMasterKeys[++nMasterKeyMaxID] = kMasterKey;
        WalletBatch* encrypted_batch = new WalletBatch(GetDatabase());
        if (!encrypted_batch->TxnBegin()) {
            delete encrypted_batch;
            encrypted_batch = nullptr;
            return false;
        }
        encrypted_batch->WriteMasterKey(nMasterKeyMaxID, kMasterKey);
        for (const auto& spk_man_pair : m_spk_managers) {
            auto spk_man = spk_man_pair.second.get();
            if (!spk_man->Encrypt(_vMasterKey, encrypted_batch)) {
                encrypted_batch->TxnAbort();
                delete encrypted_batch;
                encrypted_batch = nullptr;
                assert(false);
            }
        }
        SetMinVersion(FEATURE_WALLETCRYPT, encrypted_batch);
        if (!encrypted_batch->TxnCommit()) {
            delete encrypted_batch;
            encrypted_batch = nullptr;
            assert(false);
        }
        delete encrypted_batch;
        encrypted_batch = nullptr;
        Lock();
        Unlock(strWalletPassphrase);
        if (IsWalletFlagSet(WALLET_FLAG_DESCRIPTORS) && !IsWalletFlagSet(WALLET_FLAG_BLANK_WALLET)) {
            SetupDescriptorScriptPubKeyMans();
        } else if (auto spk_man = GetLegacyScriptPubKeyMan()) {
            if (spk_man->IsHDEnabled()) {
                if (!spk_man->SetupGeneration(true)) {
                    return false;
                }
            }
        }
        Lock();
        GetDatabase().Rewrite();
        GetDatabase().ReloadDbEnv();
    }
    NotifyStatusChanged(this);
    return true;
}
DBErrors CWallet::ReorderTransactions()
{
    LOCK(cs_wallet);
    WalletBatch batch(GetDatabase());
    typedef std::multimap<int64_t, CWalletTx*> TxItems;
    TxItems txByTime;
    for (auto& entry : mapWallet)
    {
        CWalletTx* wtx = &entry.second;
        txByTime.insert(std::make_pair(wtx->nTimeReceived, wtx));
    }
    nOrderPosNext = 0;
    std::vector<int64_t> nOrderPosOffsets;
    for (TxItems::iterator it = txByTime.begin(); it != txByTime.end(); ++it)
    {
        CWalletTx *const pwtx = (*it).second;
        int64_t& nOrderPos = pwtx->nOrderPos;
        if (nOrderPos == -1)
        {
            nOrderPos = nOrderPosNext++;
            nOrderPosOffsets.push_back(nOrderPos);
            if (!batch.WriteTx(*pwtx))
                return DBErrors::LOAD_FAIL;
        }
        else
        {
            int64_t nOrderPosOff = 0;
            for (const int64_t& nOffsetStart : nOrderPosOffsets)
            {
                if (nOrderPos >= nOffsetStart)
                    ++nOrderPosOff;
            }
            nOrderPos += nOrderPosOff;
            nOrderPosNext = std::max(nOrderPosNext, nOrderPos + 1);
            if (!nOrderPosOff)
                continue;
            if (!batch.WriteTx(*pwtx))
                return DBErrors::LOAD_FAIL;
        }
    }
    batch.WriteOrderPosNext(nOrderPosNext);
    return DBErrors::LOAD_OK;
}
int64_t CWallet::IncOrderPosNext(WalletBatch* batch)
{
    AssertLockHeld(cs_wallet);
    int64_t nRet = nOrderPosNext++;
    if (batch) {
        batch->WriteOrderPosNext(nOrderPosNext);
    } else {
        WalletBatch(GetDatabase()).WriteOrderPosNext(nOrderPosNext);
    }
    return nRet;
}
void CWallet::MarkDirty()
{
    {
        LOCK(cs_wallet);
        for (std::pair<const uint256, CWalletTx>& item : mapWallet)
            item.second.MarkDirty();
    }
}
bool CWallet::MarkReplaced(const uint256& originalHash, const uint256& newHash)
{
    LOCK(cs_wallet);
    auto mi = mapWallet.find(originalHash);
    assert(mi != mapWallet.end());
    CWalletTx& wtx = (*mi).second;
    assert(wtx.mapValue.count("replaced_by_txid") == 0);
    wtx.mapValue["replaced_by_txid"] = newHash.ToString();
    RefreshMempoolStatus(wtx, chain());
    WalletBatch batch(GetDatabase());
    bool success = true;
    if (!batch.WriteTx(wtx)) {
        WalletLogPrintf("%s: Updating batch tx %s failed\n", __func__, wtx.GetHash().ToString());
        success = false;
    }
    NotifyTransactionChanged(originalHash, CT_UPDATED);
    return success;
}
void CWallet::SetSpentKeyState(WalletBatch& batch, const uint256& hash, unsigned int n, bool used, std::set<CTxDestination>& tx_destinations)
{
    AssertLockHeld(cs_wallet);
    const CWalletTx* srctx = GetWalletTx(hash);
    if (!srctx) return;
    CTxDestination dst;
    if (ExtractDestination(srctx->tx->vout[n].scriptPubKey, dst)) {
        if (IsMine(dst)) {
            if (used != IsAddressUsed(dst)) {
                if (used) {
                    tx_destinations.insert(dst);
                }
                SetAddressUsed(batch, dst, used);
            }
        }
    }
}
bool CWallet::IsSpentKey(const CScript& scriptPubKey) const
{
    AssertLockHeld(cs_wallet);
    CTxDestination dest;
    if (!ExtractDestination(scriptPubKey, dest)) {
        return false;
    }
    if (IsAddressUsed(dest)) {
        return true;
    }
    if (IsLegacy()) {
        LegacyScriptPubKeyMan* spk_man = GetLegacyScriptPubKeyMan();
        assert(spk_man != nullptr);
        for (const auto& keyid : GetAffectedKeys(scriptPubKey, *spk_man)) {
            WitnessV0KeyHash wpkh_dest(keyid);
            if (IsAddressUsed(wpkh_dest)) {
                return true;
            }
            ScriptHash sh_wpkh_dest(GetScriptForDestination(wpkh_dest));
            if (IsAddressUsed(sh_wpkh_dest)) {
                return true;
            }
            PKHash pkh_dest(keyid);
            if (IsAddressUsed(pkh_dest)) {
                return true;
            }
        }
    }
    return false;
}
CWalletTx* CWallet::AddToWallet(CTransactionRef tx, const TxState& state, const UpdateWalletTxFn& update_wtx, bool fFlushOnClose, bool rescanning_old_block)
{
    LOCK(cs_wallet);
    WalletBatch batch(GetDatabase(), fFlushOnClose);
    uint256 hash = tx->GetHash();
    if (IsWalletFlagSet(WALLET_FLAG_AVOID_REUSE)) {
        std::set<CTxDestination> tx_destinations;
        for (const CTxIn& txin : tx->vin) {
            const COutPoint& op = txin.prevout;
            SetSpentKeyState(batch, op.hash, op.n, true, tx_destinations);
        }
        MarkDestinationsDirty(tx_destinations);
    }
    auto ret = mapWallet.emplace(std::piecewise_construct, std::forward_as_tuple(hash), std::forward_as_tuple(tx, state));
    CWalletTx& wtx = (*ret.first).second;
    bool fInsertedNew = ret.second;
    bool fUpdated = update_wtx && update_wtx(wtx, fInsertedNew);
    if (fInsertedNew) {
        wtx.nTimeReceived = GetTime();
        wtx.nOrderPos = IncOrderPosNext(&batch);
        wtx.m_it_wtxOrdered = wtxOrdered.insert(std::make_pair(wtx.nOrderPos, &wtx));
        wtx.nTimeSmart = ComputeTimeSmart(wtx, rescanning_old_block);
        AddToSpends(wtx, &batch);
    }
    if (!fInsertedNew)
    {
        if (state.index() != wtx.m_state.index()) {
            wtx.m_state = state;
            fUpdated = true;
        } else {
            assert(TxStateSerializedIndex(wtx.m_state) == TxStateSerializedIndex(state));
            assert(TxStateSerializedBlockHash(wtx.m_state) == TxStateSerializedBlockHash(state));
        }
        if (tx->HasWitness() && !wtx.tx->HasWitness()) {
            wtx.SetTx(tx);
            fUpdated = true;
        }
    }
    WalletLogPrintf("AddToWallet %s  %s%s\n", hash.ToString(), (fInsertedNew ? "new" : ""), (fUpdated ? "update" : ""));
    if (fInsertedNew || fUpdated)
        if (!batch.WriteTx(wtx))
            return nullptr;
    wtx.MarkDirty();
    NotifyTransactionChanged(hash, fInsertedNew ? CT_NEW : CT_UPDATED);
#if HAVE_SYSTEM
    std::string strCmd = m_args.GetArg("-walletnotify", "");
    if (!strCmd.empty())
    {
        ReplaceAll(strCmd, "%s", hash.GetHex());
        if (auto* conf = wtx.state<TxStateConfirmed>())
        {
            ReplaceAll(strCmd, "%b", conf->confirmed_block_hash.GetHex());
            ReplaceAll(strCmd, "%h", ToString(conf->confirmed_block_height));
        } else {
            ReplaceAll(strCmd, "%b", "unconfirmed");
            ReplaceAll(strCmd, "%h", "-1");
        }
#ifndef WIN32
        ReplaceAll(strCmd, "%w", ShellEscape(GetName()));
#endif
        std::thread t(runCommand, strCmd);
        t.detach();
    }
#endif
    return &wtx;
}
bool CWallet::LoadToWallet(const uint256& hash, const UpdateWalletTxFn& fill_wtx)
{
    const auto& ins = mapWallet.emplace(std::piecewise_construct, std::forward_as_tuple(hash), std::forward_as_tuple(nullptr, TxStateInactive{}));
    CWalletTx& wtx = ins.first->second;
    if (!fill_wtx(wtx, ins.second)) {
        return false;
    }
    if (HaveChain()) {
        bool active;
        auto lookup_block = [&](const uint256& hash, int& height, TxState& state) {
            if (!chain().findBlock(hash, FoundBlock().inActiveChain(active).height(height)) || !active) {
                state = TxStateInactive{};
            }
        };
        if (auto* conf = wtx.state<TxStateConfirmed>()) {
            lookup_block(conf->confirmed_block_hash, conf->confirmed_block_height, wtx.m_state);
        } else if (auto* conf = wtx.state<TxStateConflicted>()) {
            lookup_block(conf->conflicting_block_hash, conf->conflicting_block_height, wtx.m_state);
        }
    }
    if (  ins.second) {
        wtx.m_it_wtxOrdered = wtxOrdered.insert(std::make_pair(wtx.nOrderPos, &wtx));
    }
    AddToSpends(wtx);
    for (const CTxIn& txin : wtx.tx->vin) {
        auto it = mapWallet.find(txin.prevout.hash);
        if (it != mapWallet.end()) {
            CWalletTx& prevtx = it->second;
            if (auto* prev = prevtx.state<TxStateConflicted>()) {
                MarkConflicted(prev->conflicting_block_hash, prev->conflicting_block_height, wtx.GetHash());
            }
        }
    }
    return true;
}
bool CWallet::AddToWalletIfInvolvingMe(const CTransactionRef& ptx, const SyncTxState& state, bool fUpdate, bool rescanning_old_block)
{
    const CTransaction& tx = *ptx;
    {
        AssertLockHeld(cs_wallet);
        if (auto* conf = std::get_if<TxStateConfirmed>(&state)) {
            for (const CTxIn& txin : tx.vin) {
                std::pair<TxSpends::const_iterator, TxSpends::const_iterator> range = mapTxSpends.equal_range(txin.prevout);
                while (range.first != range.second) {
                    if (range.first->second != tx.GetHash()) {
                        WalletLogPrintf("Transaction %s (in block %s) conflicts with wallet transaction %s (both spend %s:%i)\n", tx.GetHash().ToString(), conf->confirmed_block_hash.ToString(), range.first->second.ToString(), range.first->first.hash.ToString(), range.first->first.n);
                        MarkConflicted(conf->confirmed_block_hash, conf->confirmed_block_height, range.first->second);
                    }
                    range.first++;
                }
            }
        }
        bool fExisted = mapWallet.count(tx.GetHash()) != 0;
        if (fExisted && !fUpdate) return false;
        if (fExisted || IsMine(tx) || IsFromMe(tx))
        {
            for (const CTxOut& txout: tx.vout) {
                for (const auto& spk_man : GetScriptPubKeyMans(txout.scriptPubKey)) {
                    for (auto &dest : spk_man->MarkUnusedAddresses(txout.scriptPubKey)) {
                        if (!dest.internal.has_value()) {
                            dest.internal = IsInternalScriptPubKeyMan(spk_man);
                        }
                        if (!dest.internal.has_value()) continue;
                        if (!*dest.internal && !FindAddressBookEntry(dest.dest,   false)) {
                            SetAddressBook(dest.dest, "", "receive");
                        }
                    }
                }
            }
            TxState tx_state = std::visit([](auto&& s) -> TxState { return s; }, state);
            CWalletTx* wtx = AddToWallet(MakeTransactionRef(tx), tx_state,  nullptr,  false, rescanning_old_block);
            if (!wtx) {
                throw std::runtime_error("DB error adding transaction to wallet, write failed");
            }
            return true;
        }
    }
    return false;
}
bool CWallet::TransactionCanBeAbandoned(const uint256& hashTx) const
{
    LOCK(cs_wallet);
    const CWalletTx* wtx = GetWalletTx(hashTx);
    return wtx && !wtx->isAbandoned() && GetTxDepthInMainChain(*wtx) == 0 && !wtx->InMempool();
}
void CWallet::MarkInputsDirty(const CTransactionRef& tx)
{
    for (const CTxIn& txin : tx->vin) {
        auto it = mapWallet.find(txin.prevout.hash);
        if (it != mapWallet.end()) {
            it->second.MarkDirty();
        }
    }
}
bool CWallet::AbandonTransaction(const uint256& hashTx)
{
    LOCK(cs_wallet);
    WalletBatch batch(GetDatabase());
    std::set<uint256> todo;
    std::set<uint256> done;
    auto it = mapWallet.find(hashTx);
    assert(it != mapWallet.end());
    const CWalletTx& origtx = it->second;
    if (GetTxDepthInMainChain(origtx) != 0 || origtx.InMempool()) {
        return false;
    }
    todo.insert(hashTx);
    while (!todo.empty()) {
        uint256 now = *todo.begin();
        todo.erase(now);
        done.insert(now);
        auto it = mapWallet.find(now);
        assert(it != mapWallet.end());
        CWalletTx& wtx = it->second;
        int currentconfirm = GetTxDepthInMainChain(wtx);
        assert(currentconfirm <= 0);
        if (currentconfirm == 0 && !wtx.isAbandoned()) {
            assert(!wtx.InMempool());
            wtx.m_state = TxStateInactive{ true};
            wtx.MarkDirty();
            batch.WriteTx(wtx);
            NotifyTransactionChanged(wtx.GetHash(), CT_UPDATED);
            for (unsigned int i = 0; i < wtx.tx->vout.size(); ++i) {
                std::pair<TxSpends::const_iterator, TxSpends::const_iterator> range = mapTxSpends.equal_range(COutPoint(now, i));
                for (TxSpends::const_iterator iter = range.first; iter != range.second; ++iter) {
                    if (!done.count(iter->second)) {
                        todo.insert(iter->second);
                    }
                }
            }
            MarkInputsDirty(wtx.tx);
        }
    }
    return true;
}
void CWallet::MarkConflicted(const uint256& hashBlock, int conflicting_height, const uint256& hashTx)
{
    LOCK(cs_wallet);
    int conflictconfirms = (m_last_block_processed_height - conflicting_height + 1) * -1;
    if (conflictconfirms >= 0)
        return;
    WalletBatch batch(GetDatabase(), false);
    std::set<uint256> todo;
    std::set<uint256> done;
    todo.insert(hashTx);
    while (!todo.empty()) {
        uint256 now = *todo.begin();
        todo.erase(now);
        done.insert(now);
        auto it = mapWallet.find(now);
        assert(it != mapWallet.end());
        CWalletTx& wtx = it->second;
        int currentconfirm = GetTxDepthInMainChain(wtx);
        if (conflictconfirms < currentconfirm) {
            wtx.m_state = TxStateConflicted{hashBlock, conflicting_height};
            wtx.MarkDirty();
            batch.WriteTx(wtx);
            for (unsigned int i = 0; i < wtx.tx->vout.size(); ++i) {
                std::pair<TxSpends::const_iterator, TxSpends::const_iterator> range = mapTxSpends.equal_range(COutPoint(now, i));
                for (TxSpends::const_iterator iter = range.first; iter != range.second; ++iter) {
                    if (!done.count(iter->second)) {
                        todo.insert(iter->second);
                    }
                }
            }
            MarkInputsDirty(wtx.tx);
        }
    }
}
void CWallet::SyncTransaction(const CTransactionRef& ptx, const SyncTxState& state, bool update_tx, bool rescanning_old_block)
{
    if (!AddToWalletIfInvolvingMe(ptx, state, update_tx, rescanning_old_block))
        return;
    MarkInputsDirty(ptx);
}
void CWallet::transactionAddedToMempool(const CTransactionRef& tx, uint64_t mempool_sequence) {
    LOCK(cs_wallet);
    SyncTransaction(tx, TxStateInMempool{});
    auto it = mapWallet.find(tx->GetHash());
    if (it != mapWallet.end()) {
        RefreshMempoolStatus(it->second, chain());
    }
}
void CWallet::transactionRemovedFromMempool(const CTransactionRef& tx, MemPoolRemovalReason reason, uint64_t mempool_sequence) {
    LOCK(cs_wallet);
    auto it = mapWallet.find(tx->GetHash());
    if (it != mapWallet.end()) {
        RefreshMempoolStatus(it->second, chain());
    }
    if (reason == MemPoolRemovalReason::CONFLICT) {
        SyncTransaction(tx, TxStateInactive{});
    }
}
void CWallet::blockConnected(const interfaces::BlockInfo& block)
{
    assert(block.data);
    LOCK(cs_wallet);
    m_last_block_processed_height = block.height;
    m_last_block_processed = block.hash;
    for (size_t index = 0; index < block.data->vtx.size(); index++) {
        SyncTransaction(block.data->vtx[index], TxStateConfirmed{block.hash, block.height, static_cast<int>(index)});
        transactionRemovedFromMempool(block.data->vtx[index], MemPoolRemovalReason::BLOCK, 0  );
    }
}
void CWallet::blockDisconnected(const interfaces::BlockInfo& block)
{
    assert(block.data);
    LOCK(cs_wallet);
    m_last_block_processed_height = block.height - 1;
    m_last_block_processed = *Assert(block.prev_hash);
    for (const CTransactionRef& ptx : Assert(block.data)->vtx) {
        SyncTransaction(ptx, TxStateInactive{});
    }
}
void CWallet::updatedBlockTip()
{
    m_best_block_time = GetTime();
}
void CWallet::BlockUntilSyncedToCurrentChain() const {
    AssertLockNotHeld(cs_wallet);
    uint256 last_block_hash = WITH_LOCK(cs_wallet, return m_last_block_processed);
    chain().waitForNotificationsIfTipChanged(last_block_hash);
}
CAmount CWallet::GetDebit(const CTxIn &txin, const isminefilter& filter) const
{
    {
        LOCK(cs_wallet);
        const auto mi = mapWallet.find(txin.prevout.hash);
        if (mi != mapWallet.end())
        {
            const CWalletTx& prev = (*mi).second;
            if (txin.prevout.n < prev.tx->vout.size())
                if (IsMine(prev.tx->vout[txin.prevout.n]) & filter)
                    return prev.tx->vout[txin.prevout.n].nValue;
        }
    }
    return 0;
}
isminetype CWallet::IsMine(const CTxOut& txout) const
{
    AssertLockHeld(cs_wallet);
    return IsMine(txout.scriptPubKey);
}
isminetype CWallet::IsMine(const CTxDestination& dest) const
{
    AssertLockHeld(cs_wallet);
    return IsMine(GetScriptForDestination(dest));
}
isminetype CWallet::IsMine(const CScript& script) const
{
    AssertLockHeld(cs_wallet);
    isminetype result = ISMINE_NO;
    for (const auto& spk_man_pair : m_spk_managers) {
        result = std::max(result, spk_man_pair.second->IsMine(script));
    }
    return result;
}
bool CWallet::IsMine(const CTransaction& tx) const
{
    AssertLockHeld(cs_wallet);
    for (const CTxOut& txout : tx.vout)
        if (IsMine(txout))
            return true;
    return false;
}
isminetype CWallet::IsMine(const COutPoint& outpoint) const
{
    AssertLockHeld(cs_wallet);
    auto wtx = GetWalletTx(outpoint.hash);
    if (!wtx) {
        return ISMINE_NO;
    }
    if (outpoint.n >= wtx->tx->vout.size()) {
        return ISMINE_NO;
    }
    return IsMine(wtx->tx->vout[outpoint.n]);
}
bool CWallet::IsFromMe(const CTransaction& tx) const
{
    return (GetDebit(tx, ISMINE_ALL) > 0);
}
CAmount CWallet::GetDebit(const CTransaction& tx, const isminefilter& filter) const
{
    CAmount nDebit = 0;
    for (const CTxIn& txin : tx.vin)
    {
        nDebit += GetDebit(txin, filter);
        if (!MoneyRange(nDebit))
            throw std::runtime_error(std::string(__func__) + ": value out of range");
    }
    return nDebit;
}
bool CWallet::IsHDEnabled() const
{
    bool result = false;
    for (const auto& spk_man : GetActiveScriptPubKeyMans()) {
        if (!spk_man->IsHDEnabled()) return false;
        result = true;
    }
    return result;
}
bool CWallet::CanGetAddresses(bool internal) const
{
    LOCK(cs_wallet);
    if (m_spk_managers.empty()) return false;
    for (OutputType t : OUTPUT_TYPES) {
        auto spk_man = GetScriptPubKeyMan(t, internal);
        if (spk_man && spk_man->CanGetAddresses(internal)) {
            return true;
        }
    }
    return false;
}
void CWallet::SetWalletFlag(uint64_t flags)
{
    LOCK(cs_wallet);
    m_wallet_flags |= flags;
    if (!WalletBatch(GetDatabase()).WriteWalletFlags(m_wallet_flags))
        throw std::runtime_error(std::string(__func__) + ": writing wallet flags failed");
}
void CWallet::UnsetWalletFlag(uint64_t flag)
{
    WalletBatch batch(GetDatabase());
    UnsetWalletFlagWithDB(batch, flag);
}
void CWallet::UnsetWalletFlagWithDB(WalletBatch& batch, uint64_t flag)
{
    LOCK(cs_wallet);
    m_wallet_flags &= ~flag;
    if (!batch.WriteWalletFlags(m_wallet_flags))
        throw std::runtime_error(std::string(__func__) + ": writing wallet flags failed");
}
void CWallet::UnsetBlankWalletFlag(WalletBatch& batch)
{
    UnsetWalletFlagWithDB(batch, WALLET_FLAG_BLANK_WALLET);
}
bool CWallet::IsWalletFlagSet(uint64_t flag) const
{
    return (m_wallet_flags & flag);
}
bool CWallet::LoadWalletFlags(uint64_t flags)
{
    LOCK(cs_wallet);
    if (((flags & KNOWN_WALLET_FLAGS) >> 32) ^ (flags >> 32)) {
        return false;
    }
    m_wallet_flags = flags;
    return true;
}
void CWallet::InitWalletFlags(uint64_t flags)
{
    LOCK(cs_wallet);
    assert(((flags & KNOWN_WALLET_FLAGS) >> 32) == (flags >> 32));
    assert(m_wallet_flags == 0);
    if (!WalletBatch(GetDatabase()).WriteWalletFlags(flags)) {
        throw std::runtime_error(std::string(__func__) + ": writing wallet flags failed");
    }
    if (!LoadWalletFlags(flags)) assert(false);
}
bool DummySignInput(const SigningProvider& provider, CTxIn &tx_in, const CTxOut &txout, const CCoinControl* coin_control)
{
    const CScript& scriptPubKey = txout.scriptPubKey;
    SignatureData sigdata;
    const bool use_max_sig = coin_control && (coin_control->fAllowWatchOnly || coin_control->IsExternalSelected(tx_in.prevout));
    if (!ProduceSignature(provider, use_max_sig ? DUMMY_MAXIMUM_SIGNATURE_CREATOR : DUMMY_SIGNATURE_CREATOR, scriptPubKey, sigdata)) {
        return false;
    }
    UpdateInput(tx_in, sigdata);
    return true;
}
bool FillInputToWeight(CTxIn& txin, int64_t target_weight)
{
    assert(txin.scriptSig.empty());
    assert(txin.scriptWitness.IsNull());
    int64_t txin_weight = GetTransactionInputWeight(txin);
    if (target_weight < txin_weight) {
        return false;
    }
    if (target_weight == txin_weight) {
        return true;
    }
    int64_t add_weight = target_weight - txin_weight;
    assert(add_weight > 0);
    if ((add_weight >= 253 && add_weight < 263)
        || (add_weight > std::numeric_limits<uint16_t>::max() && add_weight <= std::numeric_limits<uint16_t>::max() + 10)
        || (add_weight > std::numeric_limits<uint32_t>::max() && add_weight <= std::numeric_limits<uint32_t>::max() + 10)) {
        int64_t first_weight = add_weight / 3;
        add_weight -= first_weight;
        first_weight -= GetSizeOfCompactSize(first_weight);
        txin.scriptWitness.stack.emplace(txin.scriptWitness.stack.end(), first_weight, 0);
    }
    add_weight -= GetSizeOfCompactSize(add_weight);
    txin.scriptWitness.stack.emplace(txin.scriptWitness.stack.end(), add_weight, 0);
    assert(GetTransactionInputWeight(txin) == target_weight);
    return true;
}
bool CWallet::DummySignTx(CMutableTransaction &txNew, const std::vector<CTxOut> &txouts, const CCoinControl* coin_control) const
{
    int nIn = 0;
    for (const auto& txout : txouts)
    {
        CTxIn& txin = txNew.vin[nIn];
        if (coin_control && coin_control->HasInputWeight(txin.prevout)) {
            if (!FillInputToWeight(txin, coin_control->GetInputWeight(txin.prevout))) {
                return false;
            }
            nIn++;
            continue;
        }
        const std::unique_ptr<SigningProvider> provider = GetSolvingProvider(txout.scriptPubKey);
        if (!provider || !DummySignInput(*provider, txin, txout, coin_control)) {
            if (!coin_control || !DummySignInput(coin_control->m_external_provider, txin, txout, coin_control)) {
                return false;
            }
        }
        nIn++;
    }
    return true;
}
bool CWallet::ImportScripts(const std::set<CScript> scripts, int64_t timestamp)
{
    auto spk_man = GetLegacyScriptPubKeyMan();
    if (!spk_man) {
        return false;
    }
    LOCK(spk_man->cs_KeyStore);
    return spk_man->ImportScripts(scripts, timestamp);
}
bool CWallet::ImportPrivKeys(const std::map<CKeyID, CKey>& privkey_map, const int64_t timestamp)
{
    auto spk_man = GetLegacyScriptPubKeyMan();
    if (!spk_man) {
        return false;
    }
    LOCK(spk_man->cs_KeyStore);
    return spk_man->ImportPrivKeys(privkey_map, timestamp);
}
bool CWallet::ImportPubKeys(const std::vector<CKeyID>& ordered_pubkeys, const std::map<CKeyID, CPubKey>& pubkey_map, const std::map<CKeyID, std::pair<CPubKey, KeyOriginInfo>>& key_origins, const bool add_keypool, const bool internal, const int64_t timestamp)
{
    auto spk_man = GetLegacyScriptPubKeyMan();
    if (!spk_man) {
        return false;
    }
    LOCK(spk_man->cs_KeyStore);
    return spk_man->ImportPubKeys(ordered_pubkeys, pubkey_map, key_origins, add_keypool, internal, timestamp);
}
bool CWallet::ImportScriptPubKeys(const std::string& label, const std::set<CScript>& script_pub_keys, const bool have_solving_data, const bool apply_label, const int64_t timestamp)
{
    auto spk_man = GetLegacyScriptPubKeyMan();
    if (!spk_man) {
        return false;
    }
    LOCK(spk_man->cs_KeyStore);
    if (!spk_man->ImportScriptPubKeys(script_pub_keys, have_solving_data, timestamp)) {
        return false;
    }
    if (apply_label) {
        WalletBatch batch(GetDatabase());
        for (const CScript& script : script_pub_keys) {
            CTxDestination dest;
            ExtractDestination(script, dest);
            if (IsValidDestination(dest)) {
                SetAddressBookWithDB(batch, dest, label, "receive");
            }
        }
    }
    return true;
}
int64_t CWallet::RescanFromTime(int64_t startTime, const WalletRescanReserver& reserver, bool update)
{
    int start_height = 0;
    uint256 start_block;
    bool start = chain().findFirstBlockWithTimeAndHeight(startTime - TIMESTAMP_WINDOW, 0, FoundBlock().hash(start_block).height(start_height));
    WalletLogPrintf("%s: Rescanning last %i blocks\n", __func__, start ? WITH_LOCK(cs_wallet, return GetLastBlockHeight()) - start_height + 1 : 0);
    if (start) {
        ScanResult result = ScanForWalletTransactions(start_block, start_height,  {}, reserver,  update,  false);
        if (result.status == ScanResult::FAILURE) {
            int64_t time_max;
            CHECK_NONFATAL(chain().findBlock(result.last_failed_block, FoundBlock().maxTime(time_max)));
            return time_max + TIMESTAMP_WINDOW + 1;
        }
    }
    return startTime;
}
CWallet::ScanResult CWallet::ScanForWalletTransactions(const uint256& start_block, int start_height, std::optional<int> max_height, const WalletRescanReserver& reserver, bool fUpdate, const bool save_progress)
{
    constexpr auto INTERVAL_TIME{60s};
    auto current_time{reserver.now()};
    auto start_time{reserver.now()};
    assert(reserver.isReserved());
    uint256 block_hash = start_block;
    ScanResult result;
    WalletLogPrintf("Rescan started from block %s...\n", start_block.ToString());
    fAbortRescan = false;
    ShowProgress(strprintf("%s " + _("Rescanning…").translated, GetDisplayName()), 0);
    uint256 tip_hash = WITH_LOCK(cs_wallet, return GetLastBlockHash());
    uint256 end_hash = tip_hash;
    if (max_height) chain().findAncestorByHeight(tip_hash, *max_height, FoundBlock().hash(end_hash));
    double progress_begin = chain().guessVerificationProgress(block_hash);
    double progress_end = chain().guessVerificationProgress(end_hash);
    double progress_current = progress_begin;
    int block_height = start_height;
    while (!fAbortRescan && !chain().shutdownRequested()) {
        if (progress_end - progress_begin > 0.0) {
            m_scanning_progress = (progress_current - progress_begin) / (progress_end - progress_begin);
        } else {
            m_scanning_progress = 0;
        }
        if (block_height % 100 == 0 && progress_end - progress_begin > 0.0) {
            ShowProgress(strprintf("%s " + _("Rescanning…").translated, GetDisplayName()), std::max(1, std::min(99, (int)(m_scanning_progress * 100))));
        }
        bool next_interval = reserver.now() >= current_time + INTERVAL_TIME;
        if (next_interval) {
            current_time = reserver.now();
            WalletLogPrintf("Still rescanning. At block %d. Progress=%f\n", block_height, progress_current);
        }
        CBlock block;
        chain().findBlock(block_hash, FoundBlock().data(block));
        bool block_still_active = false;
        bool next_block = false;
        uint256 next_block_hash;
        chain().findBlock(block_hash, FoundBlock().inActiveChain(block_still_active).nextBlock(FoundBlock().inActiveChain(next_block).hash(next_block_hash)));
        if (!block.IsNull()) {
            LOCK(cs_wallet);
            if (!block_still_active) {
                result.last_failed_block = block_hash;
                result.status = ScanResult::FAILURE;
                break;
            }
            for (size_t posInBlock = 0; posInBlock < block.vtx.size(); ++posInBlock) {
                SyncTransaction(block.vtx[posInBlock], TxStateConfirmed{block_hash, block_height, static_cast<int>(posInBlock)}, fUpdate,  true);
            }
            result.last_scanned_block = block_hash;
            result.last_scanned_height = block_height;
            if (save_progress && next_interval) {
                CBlockLocator loc = m_chain->getActiveChainLocator(block_hash);
                if (!loc.IsNull()) {
                    WalletLogPrintf("Saving scan progress %d.\n", block_height);
                    WalletBatch batch(GetDatabase());
                    batch.WriteBestBlock(loc);
                }
            }
        } else {
            result.last_failed_block = block_hash;
            result.status = ScanResult::FAILURE;
        }
        if (max_height && block_height >= *max_height) {
            break;
        }
        {
            if (!next_block) {
                break;
            }
            block_hash = next_block_hash;
            ++block_height;
            progress_current = chain().guessVerificationProgress(block_hash);
            const uint256 prev_tip_hash = tip_hash;
            tip_hash = WITH_LOCK(cs_wallet, return GetLastBlockHash());
            if (!max_height && prev_tip_hash != tip_hash) {
                progress_end = chain().guessVerificationProgress(tip_hash);
            }
        }
    }
    if (!max_height) {
        WalletLogPrintf("Scanning current mempool transactions.\n");
        WITH_LOCK(cs_wallet, chain().requestMempoolTransactions(*this));
    }
    ShowProgress(strprintf("%s " + _("Rescanning…").translated, GetDisplayName()), 100);
    if (block_height && fAbortRescan) {
        WalletLogPrintf("Rescan aborted at block %d. Progress=%f\n", block_height, progress_current);
        result.status = ScanResult::USER_ABORT;
    } else if (block_height && chain().shutdownRequested()) {
        WalletLogPrintf("Rescan interrupted by shutdown request at block %d. Progress=%f\n", block_height, progress_current);
        result.status = ScanResult::USER_ABORT;
    } else {
        WalletLogPrintf("Rescan completed in %15dms\n", Ticks<std::chrono::milliseconds>(reserver.now() - start_time));
    }
    return result;
}
bool CWallet::SubmitTxMemoryPoolAndRelay(CWalletTx& wtx, std::string& err_string, bool relay) const
{
    AssertLockHeld(cs_wallet);
    if (!GetBroadcastTransactions()) return false;
    if (wtx.isAbandoned()) return false;
    if (wtx.IsCoinBase()) return false;
    if (GetTxDepthInMainChain(wtx) != 0) return false;
    WalletLogPrintf("Submitting wtx %s to mempool for relay\n", wtx.GetHash().ToString());
    bool ret = chain().broadcastTransaction(wtx.tx, m_default_max_tx_fee, relay, err_string);
    if (ret) wtx.m_state = TxStateInMempool{};
    return ret;
}
std::set<uint256> CWallet::GetTxConflicts(const CWalletTx& wtx) const
{
    AssertLockHeld(cs_wallet);
    const uint256 myHash{wtx.GetHash()};
    std::set<uint256> result{GetConflicts(myHash)};
    result.erase(myHash);
    return result;
}
void CWallet::ResubmitWalletTransactions(bool relay, bool force)
{
    if (!fBroadcastTransactions) return;
    if (!force && relay && !chain().isReadyToBroadcast()) return;
    if (!force && GetTime() < m_next_resend) return;
    m_next_resend = GetTime() + (12 * 60 * 60) + GetRand(24 * 60 * 60);
    int submitted_tx_count = 0;
    {
        LOCK(cs_wallet);
        std::set<CWalletTx*, WalletTxOrderComparator> to_submit;
        for (auto& [txid, wtx] : mapWallet) {
            if (!wtx.isUnconfirmed()) continue;
            if (!force && wtx.nTimeReceived > m_best_block_time - 5 * 60) continue;
            to_submit.insert(&wtx);
        }
        for (auto wtx : to_submit) {
            std::string unused_err_string;
            if (SubmitTxMemoryPoolAndRelay(*wtx, unused_err_string, relay)) ++submitted_tx_count;
        }
    }
    if (submitted_tx_count > 0) {
        WalletLogPrintf("%s: resubmit %u unconfirmed transactions\n", __func__, submitted_tx_count);
    }
}
void MaybeResendWalletTxs(WalletContext& context)
{
    for (const std::shared_ptr<CWallet>& pwallet : GetWallets(context)) {
        pwallet->ResubmitWalletTransactions( true,  false);
    }
}
bool CWallet::SignTransaction(CMutableTransaction& tx) const
{
    AssertLockHeld(cs_wallet);
    std::map<COutPoint, Coin> coins;
    for (auto& input : tx.vin) {
        const auto mi = mapWallet.find(input.prevout.hash);
        if(mi == mapWallet.end() || input.prevout.n >= mi->second.tx->vout.size()) {
            return false;
        }
        const CWalletTx& wtx = mi->second;
        int prev_height = wtx.state<TxStateConfirmed>() ? wtx.state<TxStateConfirmed>()->confirmed_block_height : 0;
        coins[input.prevout] = Coin(wtx.tx->vout[input.prevout.n], prev_height, wtx.IsCoinBase());
    }
    std::map<int, bilingual_str> input_errors;
    return SignTransaction(tx, coins, SIGHASH_DEFAULT, input_errors);
}
bool CWallet::SignTransaction(CMutableTransaction& tx, const std::map<COutPoint, Coin>& coins, int sighash, std::map<int, bilingual_str>& input_errors) const
{
    for (ScriptPubKeyMan* spk_man : GetAllScriptPubKeyMans()) {
        if (spk_man->SignTransaction(tx, coins, sighash, input_errors)) {
            return true;
        }
    }
    return false;
}
TransactionError CWallet::FillPSBT(PartiallySignedTransaction& psbtx, bool& complete, int sighash_type, bool sign, bool bip32derivs, size_t * n_signed, bool finalize) const
{
    if (n_signed) {
        *n_signed = 0;
    }
    LOCK(cs_wallet);
    for (unsigned int i = 0; i < psbtx.tx->vin.size(); ++i) {
        const CTxIn& txin = psbtx.tx->vin[i];
        PSBTInput& input = psbtx.inputs.at(i);
        if (PSBTInputSigned(input)) {
            continue;
        }
        if (!input.non_witness_utxo) {
            const uint256& txhash = txin.prevout.hash;
            const auto it = mapWallet.find(txhash);
            if (it != mapWallet.end()) {
                const CWalletTx& wtx = it->second;
                input.non_witness_utxo = wtx.tx;
            }
        }
    }
    const PrecomputedTransactionData txdata = PrecomputePSBTData(psbtx);
    for (ScriptPubKeyMan* spk_man : GetAllScriptPubKeyMans()) {
        int n_signed_this_spkm = 0;
        TransactionError res = spk_man->FillPSBT(psbtx, txdata, sighash_type, sign, bip32derivs, &n_signed_this_spkm, finalize);
        if (res != TransactionError::OK) {
            return res;
        }
        if (n_signed) {
            (*n_signed) += n_signed_this_spkm;
        }
    }
    if ((sighash_type & 0x80) != SIGHASH_ANYONECANPAY) {
        std::vector<unsigned int> to_drop;
        for (unsigned int i = 0; i < psbtx.inputs.size(); ++i) {
            const auto& input = psbtx.inputs.at(i);
            int wit_ver;
            std::vector<unsigned char> wit_prog;
            if (input.witness_utxo.IsNull() || !input.witness_utxo.scriptPubKey.IsWitnessProgram(wit_ver, wit_prog)) {
                to_drop.clear();
                break;
            }
            if (wit_ver == 0) {
                to_drop.clear();
                break;
            }
            if (input.non_witness_utxo) {
                to_drop.push_back(i);
            }
        }
        for (unsigned int i : to_drop) {
            psbtx.inputs.at(i).non_witness_utxo = nullptr;
        }
    }
    complete = true;
    for (const auto& input : psbtx.inputs) {
        complete &= PSBTInputSigned(input);
    }
    return TransactionError::OK;
}
SigningResult CWallet::SignMessage(const std::string& message, const PKHash& pkhash, std::string& str_sig) const
{
    SignatureData sigdata;
    CScript script_pub_key = GetScriptForDestination(pkhash);
    for (const auto& spk_man_pair : m_spk_managers) {
        if (spk_man_pair.second->CanProvide(script_pub_key, sigdata)) {
            LOCK(cs_wallet);
            return spk_man_pair.second->SignMessage(message, pkhash, str_sig);
        }
    }
    return SigningResult::PRIVATE_KEY_NOT_AVAILABLE;
}
OutputType CWallet::TransactionChangeType(const std::optional<OutputType>& change_type, const std::vector<CRecipient>& vecSend) const
{
    if (change_type) {
        return *change_type;
    }
    if (m_default_address_type == OutputType::LEGACY) {
        return OutputType::LEGACY;
    }
    bool any_tr{false};
    bool any_wpkh{false};
    bool any_sh{false};
    bool any_pkh{false};
    for (const auto& recipient : vecSend) {
        std::vector<std::vector<uint8_t>> dummy;
        const TxoutType type{Solver(recipient.scriptPubKey, dummy)};
        if (type == TxoutType::WITNESS_V1_TAPROOT) {
            any_tr = true;
        } else if (type == TxoutType::WITNESS_V0_KEYHASH) {
            any_wpkh = true;
        } else if (type == TxoutType::SCRIPTHASH) {
            any_sh = true;
        } else if (type == TxoutType::PUBKEYHASH) {
            any_pkh = true;
        }
    }
    const bool has_bech32m_spkman(GetScriptPubKeyMan(OutputType::BECH32M,  true));
    if (has_bech32m_spkman && any_tr) {
        return OutputType::BECH32M;
    }
    const bool has_bech32_spkman(GetScriptPubKeyMan(OutputType::BECH32,  true));
    if (has_bech32_spkman && any_wpkh) {
        return OutputType::BECH32;
    }
    const bool has_p2sh_segwit_spkman(GetScriptPubKeyMan(OutputType::P2SH_SEGWIT,  true));
    if (has_p2sh_segwit_spkman && any_sh) {
        return OutputType::P2SH_SEGWIT;
    }
    const bool has_legacy_spkman(GetScriptPubKeyMan(OutputType::LEGACY,  true));
    if (has_legacy_spkman && any_pkh) {
        return OutputType::LEGACY;
    }
    if (has_bech32m_spkman) {
        return OutputType::BECH32M;
    }
    if (has_bech32_spkman) {
        return OutputType::BECH32;
    }
    return m_default_address_type;
}
void CWallet::CommitTransaction(CTransactionRef tx, mapValue_t mapValue, std::vector<std::pair<std::string, std::string>> orderForm)
{
    LOCK(cs_wallet);
    WalletLogPrintf("CommitTransaction:\n%s", tx->ToString());
    CWalletTx* wtx = AddToWallet(tx, TxStateInactive{}, [&](CWalletTx& wtx, bool new_tx) {
        CHECK_NONFATAL(wtx.mapValue.empty());
        CHECK_NONFATAL(wtx.vOrderForm.empty());
        wtx.mapValue = std::move(mapValue);
        wtx.vOrderForm = std::move(orderForm);
        wtx.fTimeReceivedIsTxTime = true;
        wtx.fFromMe = true;
        return true;
    });
    if (!wtx) {
        throw std::runtime_error(std::string(__func__) + ": Wallet db error, transaction commit failed");
    }
    for (const CTxIn& txin : tx->vin) {
        CWalletTx &coin = mapWallet.at(txin.prevout.hash);
        coin.MarkDirty();
        NotifyTransactionChanged(coin.GetHash(), CT_UPDATED);
    }
    if (!fBroadcastTransactions) {
        return;
    }
    std::string err_string;
    if (!SubmitTxMemoryPoolAndRelay(*wtx, err_string, true)) {
        WalletLogPrintf("CommitTransaction(): Transaction cannot be broadcast immediately, %s\n", err_string);
    }
}
DBErrors CWallet::LoadWallet()
{
    LOCK(cs_wallet);
    DBErrors nLoadWalletRet = WalletBatch(GetDatabase()).LoadWallet(this);
    if (nLoadWalletRet == DBErrors::NEED_REWRITE)
    {
        if (GetDatabase().Rewrite("\x04pool"))
        {
            for (const auto& spk_man_pair : m_spk_managers) {
                spk_man_pair.second->RewriteDB();
            }
        }
    }
    if (m_spk_managers.empty()) {
        assert(m_external_spk_managers.empty());
        assert(m_internal_spk_managers.empty());
    }
    return nLoadWalletRet;
}
DBErrors CWallet::ZapSelectTx(std::vector<uint256>& vHashIn, std::vector<uint256>& vHashOut)
{
    AssertLockHeld(cs_wallet);
    DBErrors nZapSelectTxRet = WalletBatch(GetDatabase()).ZapSelectTx(vHashIn, vHashOut);
    for (const uint256& hash : vHashOut) {
        const auto& it = mapWallet.find(hash);
        wtxOrdered.erase(it->second.m_it_wtxOrdered);
        for (const auto& txin : it->second.tx->vin)
            mapTxSpends.erase(txin.prevout);
        mapWallet.erase(it);
        NotifyTransactionChanged(hash, CT_DELETED);
    }
    if (nZapSelectTxRet == DBErrors::NEED_REWRITE)
    {
        if (GetDatabase().Rewrite("\x04pool"))
        {
            for (const auto& spk_man_pair : m_spk_managers) {
                spk_man_pair.second->RewriteDB();
            }
        }
    }
    if (nZapSelectTxRet != DBErrors::LOAD_OK)
        return nZapSelectTxRet;
    MarkDirty();
    return DBErrors::LOAD_OK;
}
bool CWallet::SetAddressBookWithDB(WalletBatch& batch, const CTxDestination& address, const std::string& strName, const std::string& strPurpose)
{
    bool fUpdated = false;
    bool is_mine;
    {
        LOCK(cs_wallet);
        std::map<CTxDestination, CAddressBookData>::iterator mi = m_address_book.find(address);
        fUpdated = (mi != m_address_book.end() && !mi->second.IsChange());
        m_address_book[address].SetLabel(strName);
        if (!strPurpose.empty())
            m_address_book[address].purpose = strPurpose;
        is_mine = IsMine(address) != ISMINE_NO;
    }
    NotifyAddressBookChanged(address, strName, is_mine,
                             strPurpose, (fUpdated ? CT_UPDATED : CT_NEW));
    if (!strPurpose.empty() && !batch.WritePurpose(EncodeDestination(address), strPurpose))
        return false;
    return batch.WriteName(EncodeDestination(address), strName);
}
bool CWallet::SetAddressBook(const CTxDestination& address, const std::string& strName, const std::string& strPurpose)
{
    WalletBatch batch(GetDatabase());
    return SetAddressBookWithDB(batch, address, strName, strPurpose);
}
bool CWallet::DelAddressBook(const CTxDestination& address)
{
    bool is_mine;
    WalletBatch batch(GetDatabase());
    {
        LOCK(cs_wallet);
        if (IsMine(address)) {
            WalletLogPrintf("%s called with IsMine address, NOT SUPPORTED. Please report this bug! %s\n", __func__, PACKAGE_BUGREPORT);
            return false;
        }
        std::string strAddress = EncodeDestination(address);
        for (const std::pair<const std::string, std::string> &item : m_address_book[address].destdata)
        {
            batch.EraseDestData(strAddress, item.first);
        }
        m_address_book.erase(address);
        is_mine = IsMine(address) != ISMINE_NO;
    }
    NotifyAddressBookChanged(address, "", is_mine, "", CT_DELETED);
    batch.ErasePurpose(EncodeDestination(address));
    return batch.EraseName(EncodeDestination(address));
}
size_t CWallet::KeypoolCountExternalKeys() const
{
    AssertLockHeld(cs_wallet);
    auto legacy_spk_man = GetLegacyScriptPubKeyMan();
    if (legacy_spk_man) {
        return legacy_spk_man->KeypoolCountExternalKeys();
    }
    unsigned int count = 0;
    for (auto spk_man : m_external_spk_managers) {
        count += spk_man.second->GetKeyPoolSize();
    }
    return count;
}
unsigned int CWallet::GetKeyPoolSize() const
{
    AssertLockHeld(cs_wallet);
    unsigned int count = 0;
    for (auto spk_man : GetActiveScriptPubKeyMans()) {
        count += spk_man->GetKeyPoolSize();
    }
    return count;
}
bool CWallet::TopUpKeyPool(unsigned int kpSize)
{
    LOCK(cs_wallet);
    bool res = true;
    for (auto spk_man : GetActiveScriptPubKeyMans()) {
        res &= spk_man->TopUp(kpSize);
    }
    return res;
}
util::Result<CTxDestination> CWallet::GetNewDestination(const OutputType type, const std::string label)
{
    LOCK(cs_wallet);
    auto spk_man = GetScriptPubKeyMan(type, false  );
    if (!spk_man) {
        return util::Error{strprintf(_("Error: No %s addresses available."), FormatOutputType(type))};
    }
    spk_man->TopUp();
    auto op_dest = spk_man->GetNewDestination(type);
    if (op_dest) {
        SetAddressBook(*op_dest, label, "receive");
    }
    return op_dest;
}
util::Result<CTxDestination> CWallet::GetNewChangeDestination(const OutputType type)
{
    LOCK(cs_wallet);
    ReserveDestination reservedest(this, type);
    auto op_dest = reservedest.GetReservedDestination(true);
    if (op_dest) reservedest.KeepDestination();
    return op_dest;
}
std::optional<int64_t> CWallet::GetOldestKeyPoolTime() const
{
    LOCK(cs_wallet);
    if (m_spk_managers.empty()) {
        return std::nullopt;
    }
    std::optional<int64_t> oldest_key{std::numeric_limits<int64_t>::max()};
    for (const auto& spk_man_pair : m_spk_managers) {
        oldest_key = std::min(oldest_key, spk_man_pair.second->GetOldestKeyPoolTime());
    }
    return oldest_key;
}
void CWallet::MarkDestinationsDirty(const std::set<CTxDestination>& destinations) {
    for (auto& entry : mapWallet) {
        CWalletTx& wtx = entry.second;
        if (wtx.m_is_cache_empty) continue;
        for (unsigned int i = 0; i < wtx.tx->vout.size(); i++) {
            CTxDestination dst;
            if (ExtractDestination(wtx.tx->vout[i].scriptPubKey, dst) && destinations.count(dst)) {
                wtx.MarkDirty();
                break;
            }
        }
    }
}
void CWallet::ForEachAddrBookEntry(const ListAddrBookFunc& func) const
{
    AssertLockHeld(cs_wallet);
    for (const std::pair<const CTxDestination, CAddressBookData>& item : m_address_book) {
        const auto& entry = item.second;
        func(item.first, entry.GetLabel(), entry.purpose, entry.IsChange());
    }
}
std::vector<CTxDestination> CWallet::ListAddrBookAddresses(const std::optional<AddrBookFilter>& _filter) const
{
    AssertLockHeld(cs_wallet);
    std::vector<CTxDestination> result;
    AddrBookFilter filter = _filter ? *_filter : AddrBookFilter();
    ForEachAddrBookEntry([&result, &filter](const CTxDestination& dest, const std::string& label, const std::string& purpose, bool is_change) {
        if (filter.ignore_change && is_change) return;
        if (filter.m_op_label && *filter.m_op_label != label) return;
        result.emplace_back(dest);
    });
    return result;
}
std::set<std::string> CWallet::ListAddrBookLabels(const std::string& purpose) const
{
    AssertLockHeld(cs_wallet);
    std::set<std::string> label_set;
    ForEachAddrBookEntry([&](const CTxDestination& _dest, const std::string& _label,
                             const std::string& _purpose, bool _is_change) {
        if (_is_change) return;
        if (purpose.empty() || _purpose == purpose) {
            label_set.insert(_label);
        }
    });
    return label_set;
}
util::Result<CTxDestination> ReserveDestination::GetReservedDestination(bool internal)
{
    m_spk_man = pwallet->GetScriptPubKeyMan(type, internal);
    if (!m_spk_man) {
        return util::Error{strprintf(_("Error: No %s addresses available."), FormatOutputType(type))};
    }
    if (nIndex == -1)
    {
        m_spk_man->TopUp();
        CKeyPool keypool;
        auto op_address = m_spk_man->GetReservedDestination(type, internal, nIndex, keypool);
        if (!op_address) return op_address;
        address = *op_address;
        fInternal = keypool.fInternal;
    }
    return address;
}
void ReserveDestination::KeepDestination()
{
    if (nIndex != -1) {
        m_spk_man->KeepDestination(nIndex, type);
    }
    nIndex = -1;
    address = CNoDestination();
}
void ReserveDestination::ReturnDestination()
{
    if (nIndex != -1) {
        m_spk_man->ReturnDestination(nIndex, fInternal, address);
    }
    nIndex = -1;
    address = CNoDestination();
}
bool CWallet::DisplayAddress(const CTxDestination& dest)
{
    CScript scriptPubKey = GetScriptForDestination(dest);
    for (const auto& spk_man : GetScriptPubKeyMans(scriptPubKey)) {
        auto signer_spk_man = dynamic_cast<ExternalSignerScriptPubKeyMan *>(spk_man);
        if (signer_spk_man == nullptr) {
            continue;
        }
        ExternalSigner signer = ExternalSignerScriptPubKeyMan::GetExternalSigner();
        return signer_spk_man->DisplayAddress(scriptPubKey, signer);
    }
    return false;
}
bool CWallet::LockCoin(const COutPoint& output, WalletBatch* batch)
{
    AssertLockHeld(cs_wallet);
    setLockedCoins.insert(output);
    if (batch) {
        return batch->WriteLockedUTXO(output);
    }
    return true;
}
bool CWallet::UnlockCoin(const COutPoint& output, WalletBatch* batch)
{
    AssertLockHeld(cs_wallet);
    bool was_locked = setLockedCoins.erase(output);
    if (batch && was_locked) {
        return batch->EraseLockedUTXO(output);
    }
    return true;
}
bool CWallet::UnlockAllCoins()
{
    AssertLockHeld(cs_wallet);
    bool success = true;
    WalletBatch batch(GetDatabase());
    for (auto it = setLockedCoins.begin(); it != setLockedCoins.end(); ++it) {
        success &= batch.EraseLockedUTXO(*it);
    }
    setLockedCoins.clear();
    return success;
}
bool CWallet::IsLockedCoin(const COutPoint& output) const
{
    AssertLockHeld(cs_wallet);
    return setLockedCoins.count(output) > 0;
}
void CWallet::ListLockedCoins(std::vector<COutPoint>& vOutpts) const
{
    AssertLockHeld(cs_wallet);
    for (std::set<COutPoint>::iterator it = setLockedCoins.begin();
         it != setLockedCoins.end(); it++) {
        COutPoint outpt = (*it);
        vOutpts.push_back(outpt);
    }
}
void CWallet::GetKeyBirthTimes(std::map<CKeyID, int64_t>& mapKeyBirth) const {
    AssertLockHeld(cs_wallet);
    mapKeyBirth.clear();
    std::map<CKeyID, const TxStateConfirmed*> mapKeyFirstBlock;
    TxStateConfirmed max_confirm{uint256{},  -1,  -1};
    max_confirm.confirmed_block_height = GetLastBlockHeight() > 144 ? GetLastBlockHeight() - 144 : 0;
    CHECK_NONFATAL(chain().findAncestorByHeight(GetLastBlockHash(), max_confirm.confirmed_block_height, FoundBlock().hash(max_confirm.confirmed_block_hash)));
    {
        LegacyScriptPubKeyMan* spk_man = GetLegacyScriptPubKeyMan();
        assert(spk_man != nullptr);
        LOCK(spk_man->cs_KeyStore);
        for (const auto& entry : spk_man->mapKeyMetadata) {
            if (entry.second.nCreateTime) {
                mapKeyBirth[entry.first] = entry.second.nCreateTime;
            }
        }
        for (const CKeyID &keyid : spk_man->GetKeys()) {
            if (mapKeyBirth.count(keyid) == 0)
                mapKeyFirstBlock[keyid] = &max_confirm;
        }
        if (mapKeyFirstBlock.empty())
            return;
        for (const auto& entry : mapWallet) {
            const CWalletTx &wtx = entry.second;
            if (auto* conf = wtx.state<TxStateConfirmed>()) {
                for (const CTxOut &txout : wtx.tx->vout) {
                    for (const auto &keyid : GetAffectedKeys(txout.scriptPubKey, *spk_man)) {
                        auto rit = mapKeyFirstBlock.find(keyid);
                        if (rit != mapKeyFirstBlock.end() && conf->confirmed_block_height < rit->second->confirmed_block_height) {
                            rit->second = conf;
                        }
                    }
                }
            }
        }
    }
    for (const auto& entry : mapKeyFirstBlock) {
        int64_t block_time;
        CHECK_NONFATAL(chain().findBlock(entry.second->confirmed_block_hash, FoundBlock().time(block_time)));
        mapKeyBirth[entry.first] = block_time - TIMESTAMP_WINDOW;
    }
}
unsigned int CWallet::ComputeTimeSmart(const CWalletTx& wtx, bool rescanning_old_block) const
{
    std::optional<uint256> block_hash;
    if (auto* conf = wtx.state<TxStateConfirmed>()) {
        block_hash = conf->confirmed_block_hash;
    } else if (auto* conf = wtx.state<TxStateConflicted>()) {
        block_hash = conf->conflicting_block_hash;
    }
    unsigned int nTimeSmart = wtx.nTimeReceived;
    if (block_hash) {
        int64_t blocktime;
        int64_t block_max_time;
        if (chain().findBlock(*block_hash, FoundBlock().time(blocktime).maxTime(block_max_time))) {
            if (rescanning_old_block) {
                nTimeSmart = block_max_time;
            } else {
                int64_t latestNow = wtx.nTimeReceived;
                int64_t latestEntry = 0;
                int64_t latestTolerated = latestNow + 300;
                const TxItems& txOrdered = wtxOrdered;
                for (auto it = txOrdered.rbegin(); it != txOrdered.rend(); ++it) {
                    CWalletTx* const pwtx = it->second;
                    if (pwtx == &wtx) {
                        continue;
                    }
                    int64_t nSmartTime;
                    nSmartTime = pwtx->nTimeSmart;
                    if (!nSmartTime) {
                        nSmartTime = pwtx->nTimeReceived;
                    }
                    if (nSmartTime <= latestTolerated) {
                        latestEntry = nSmartTime;
                        if (nSmartTime > latestNow) {
                            latestNow = nSmartTime;
                        }
                        break;
                    }
                }
                nTimeSmart = std::max(latestEntry, std::min(blocktime, latestNow));
            }
        } else {
            WalletLogPrintf("%s: found %s in block %s not in index\n", __func__, wtx.GetHash().ToString(), block_hash->ToString());
        }
    }
    return nTimeSmart;
}
bool CWallet::SetAddressUsed(WalletBatch& batch, const CTxDestination& dest, bool used)
{
    const std::string key{"used"};
    if (std::get_if<CNoDestination>(&dest))
        return false;
    if (!used) {
        if (auto* data = util::FindKey(m_address_book, dest)) data->destdata.erase(key);
        return batch.EraseDestData(EncodeDestination(dest), key);
    }
    const std::string value{"1"};
    m_address_book[dest].destdata.insert(std::make_pair(key, value));
    return batch.WriteDestData(EncodeDestination(dest), key, value);
}
void CWallet::LoadDestData(const CTxDestination &dest, const std::string &key, const std::string &value)
{
    m_address_book[dest].destdata.insert(std::make_pair(key, value));
}
bool CWallet::IsAddressUsed(const CTxDestination& dest) const
{
    const std::string key{"used"};
    std::map<CTxDestination, CAddressBookData>::const_iterator i = m_address_book.find(dest);
    if(i != m_address_book.end())
    {
        CAddressBookData::StringMap::const_iterator j = i->second.destdata.find(key);
        if(j != i->second.destdata.end())
        {
            return true;
        }
    }
    return false;
}
std::vector<std::string> CWallet::GetAddressReceiveRequests() const
{
    const std::string prefix{"rr"};
    std::vector<std::string> values;
    for (const auto& address : m_address_book) {
        for (const auto& data : address.second.destdata) {
            if (!data.first.compare(0, prefix.size(), prefix)) {
                values.emplace_back(data.second);
            }
        }
    }
    return values;
}
bool CWallet::SetAddressReceiveRequest(WalletBatch& batch, const CTxDestination& dest, const std::string& id, const std::string& value)
{
    const std::string key{"rr" + id};
    CAddressBookData& data = m_address_book.at(dest);
    if (value.empty()) {
        if (!batch.EraseDestData(EncodeDestination(dest), key)) return false;
        data.destdata.erase(key);
    } else {
        if (!batch.WriteDestData(EncodeDestination(dest), key, value)) return false;
        data.destdata[key] = value;
    }
    return true;
}
std::unique_ptr<WalletDatabase> MakeWalletDatabase(const std::string& name, const DatabaseOptions& options, DatabaseStatus& status, bilingual_str& error_string)
{
    const fs::path wallet_path = fsbridge::AbsPathJoin(GetWalletDir(), fs::PathFromString(name));
    fs::file_type path_type = fs::symlink_status(wallet_path).type();
    if (!(path_type == fs::file_type::not_found || path_type == fs::file_type::directory ||
          (path_type == fs::file_type::symlink && fs::is_directory(wallet_path)) ||
          (path_type == fs::file_type::regular && fs::PathFromString(name).filename() == fs::PathFromString(name)))) {
        error_string = Untranslated(strprintf(
              "Invalid -wallet path '%s'. -wallet path should point to a directory where wallet.dat and "
              "database/log.?????????? files can be stored, a location where such a directory could be created, "
              "or (for backwards compatibility) the name of an existing data file in -walletdir (%s)",
              name, fs::quoted(fs::PathToString(GetWalletDir()))));
        status = DatabaseStatus::FAILED_BAD_PATH;
        return nullptr;
    }
    return MakeDatabase(wallet_path, options, status, error_string);
}
std::shared_ptr<CWallet> CWallet::Create(WalletContext& context, const std::string& name, std::unique_ptr<WalletDatabase> database, uint64_t wallet_creation_flags, bilingual_str& error, std::vector<bilingual_str>& warnings)
{
    interfaces::Chain* chain = context.chain;
    ArgsManager& args = *Assert(context.args);
    const std::string& walletFile = database->Filename();
    const auto start{SteadyClock::now()};
    const std::shared_ptr<CWallet> walletInstance(new CWallet(chain, name, args, std::move(database)), ReleaseWallet);
    bool rescan_required = false;
    DBErrors nLoadWalletRet = walletInstance->LoadWallet();
    if (nLoadWalletRet != DBErrors::LOAD_OK) {
        if (nLoadWalletRet == DBErrors::CORRUPT) {
            error = strprintf(_("Error loading %s: Wallet corrupted"), walletFile);
            return nullptr;
        }
        else if (nLoadWalletRet == DBErrors::NONCRITICAL_ERROR)
        {
            warnings.push_back(strprintf(_("Error reading %s! All keys read correctly, but transaction data"
                                           " or address book entries might be missing or incorrect."),
                walletFile));
        }
        else if (nLoadWalletRet == DBErrors::TOO_NEW) {
            error = strprintf(_("Error loading %s: Wallet requires newer version of %s"), walletFile, PACKAGE_NAME);
            return nullptr;
        }
        else if (nLoadWalletRet == DBErrors::EXTERNAL_SIGNER_SUPPORT_REQUIRED) {
            error = strprintf(_("Error loading %s: External signer wallet being loaded without external signer support compiled"), walletFile);
            return nullptr;
        }
        else if (nLoadWalletRet == DBErrors::NEED_REWRITE)
        {
            error = strprintf(_("Wallet needed to be rewritten: restart %s to complete"), PACKAGE_NAME);
            return nullptr;
        } else if (nLoadWalletRet == DBErrors::NEED_RESCAN) {
            warnings.push_back(strprintf(_("Error reading %s! Transaction data may be missing or incorrect."
                                           " Rescanning wallet."), walletFile));
            rescan_required = true;
        } else if (nLoadWalletRet == DBErrors::UNKNOWN_DESCRIPTOR) {
            error = strprintf(_("Unrecognized descriptor found. Loading wallet %s\n\n"
                                "The wallet might had been created on a newer version.\n"
                                "Please try running the latest software version.\n"), walletFile);
            return nullptr;
        } else {
            error = strprintf(_("Error loading %s"), walletFile);
            return nullptr;
        }
    }
    const bool fFirstRun = walletInstance->m_spk_managers.empty() &&
                     !walletInstance->IsWalletFlagSet(WALLET_FLAG_DISABLE_PRIVATE_KEYS) &&
                     !walletInstance->IsWalletFlagSet(WALLET_FLAG_BLANK_WALLET);
    if (fFirstRun)
    {
        walletInstance->SetMinVersion(FEATURE_LATEST);
        walletInstance->InitWalletFlags(wallet_creation_flags);
        if (!walletInstance->IsWalletFlagSet(WALLET_FLAG_DESCRIPTORS)) {
            walletInstance->SetupLegacyScriptPubKeyMan();
        }
        if ((wallet_creation_flags & WALLET_FLAG_EXTERNAL_SIGNER) || !(wallet_creation_flags & (WALLET_FLAG_DISABLE_PRIVATE_KEYS | WALLET_FLAG_BLANK_WALLET))) {
            LOCK(walletInstance->cs_wallet);
            if (walletInstance->IsWalletFlagSet(WALLET_FLAG_DESCRIPTORS)) {
                walletInstance->SetupDescriptorScriptPubKeyMans();
            } else {
                for (auto spk_man : walletInstance->GetActiveScriptPubKeyMans()) {
                    if (!spk_man->SetupGeneration()) {
                        error = _("Unable to generate initial keys");
                        return nullptr;
                    }
                }
            }
        }
        if (chain) {
            walletInstance->chainStateFlushed(chain->getTipLocator());
        }
    } else if (wallet_creation_flags & WALLET_FLAG_DISABLE_PRIVATE_KEYS) {
        error = strprintf(_("Error loading %s: Private keys can only be disabled during creation"), walletFile);
        return nullptr;
    } else if (walletInstance->IsWalletFlagSet(WALLET_FLAG_DISABLE_PRIVATE_KEYS)) {
        for (auto spk_man : walletInstance->GetActiveScriptPubKeyMans()) {
            if (spk_man->HavePrivateKeys()) {
                warnings.push_back(strprintf(_("Warning: Private keys detected in wallet {%s} with disabled private keys"), walletFile));
                break;
            }
        }
    }
    if (!args.GetArg("-addresstype", "").empty()) {
        std::optional<OutputType> parsed = ParseOutputType(args.GetArg("-addresstype", ""));
        if (!parsed) {
            error = strprintf(_("Unknown address type '%s'"), args.GetArg("-addresstype", ""));
            return nullptr;
        }
        walletInstance->m_default_address_type = parsed.value();
    }
    if (!args.GetArg("-changetype", "").empty()) {
        std::optional<OutputType> parsed = ParseOutputType(args.GetArg("-changetype", ""));
        if (!parsed) {
            error = strprintf(_("Unknown change type '%s'"), args.GetArg("-changetype", ""));
            return nullptr;
        }
        walletInstance->m_default_change_type = parsed.value();
    }
    if (args.IsArgSet("-mintxfee")) {
        std::optional<CAmount> min_tx_fee = ParseMoney(args.GetArg("-mintxfee", ""));
        if (!min_tx_fee || min_tx_fee.value() == 0) {
            error = AmountErrMsg("mintxfee", args.GetArg("-mintxfee", ""));
            return nullptr;
        } else if (min_tx_fee.value() > HIGH_TX_FEE_PER_KB) {
            warnings.push_back(AmountHighWarn("-mintxfee") + Untranslated(" ") +
                               _("This is the minimum transaction fee you pay on every transaction."));
        }
        walletInstance->m_min_fee = CFeeRate{min_tx_fee.value()};
    }
    if (args.IsArgSet("-maxapsfee")) {
        const std::string max_aps_fee{args.GetArg("-maxapsfee", "")};
        if (max_aps_fee == "-1") {
            walletInstance->m_max_aps_fee = -1;
        } else if (std::optional<CAmount> max_fee = ParseMoney(max_aps_fee)) {
            if (max_fee.value() > HIGH_APS_FEE) {
                warnings.push_back(AmountHighWarn("-maxapsfee") + Untranslated(" ") +
                                  _("This is the maximum transaction fee you pay (in addition to the normal fee) to prioritize partial spend avoidance over regular coin selection."));
            }
            walletInstance->m_max_aps_fee = max_fee.value();
        } else {
            error = AmountErrMsg("maxapsfee", max_aps_fee);
            return nullptr;
        }
    }
    if (args.IsArgSet("-fallbackfee")) {
        std::optional<CAmount> fallback_fee = ParseMoney(args.GetArg("-fallbackfee", ""));
        if (!fallback_fee) {
            error = strprintf(_("Invalid amount for -fallbackfee=<amount>: '%s'"), args.GetArg("-fallbackfee", ""));
            return nullptr;
        } else if (fallback_fee.value() > HIGH_TX_FEE_PER_KB) {
            warnings.push_back(AmountHighWarn("-fallbackfee") + Untranslated(" ") +
                               _("This is the transaction fee you may pay when fee estimates are not available."));
        }
        walletInstance->m_fallback_fee = CFeeRate{fallback_fee.value()};
    }
    walletInstance->m_allow_fallback_fee = walletInstance->m_fallback_fee.GetFeePerK() != 0;
    if (args.IsArgSet("-discardfee")) {
        std::optional<CAmount> discard_fee = ParseMoney(args.GetArg("-discardfee", ""));
        if (!discard_fee) {
            error = strprintf(_("Invalid amount for -discardfee=<amount>: '%s'"), args.GetArg("-discardfee", ""));
            return nullptr;
        } else if (discard_fee.value() > HIGH_TX_FEE_PER_KB) {
            warnings.push_back(AmountHighWarn("-discardfee") + Untranslated(" ") +
                               _("This is the transaction fee you may discard if change is smaller than dust at this level"));
        }
        walletInstance->m_discard_rate = CFeeRate{discard_fee.value()};
    }
    if (args.IsArgSet("-paytxfee")) {
        std::optional<CAmount> pay_tx_fee = ParseMoney(args.GetArg("-paytxfee", ""));
        if (!pay_tx_fee) {
            error = AmountErrMsg("paytxfee", args.GetArg("-paytxfee", ""));
            return nullptr;
        } else if (pay_tx_fee.value() > HIGH_TX_FEE_PER_KB) {
            warnings.push_back(AmountHighWarn("-paytxfee") + Untranslated(" ") +
                               _("This is the transaction fee you will pay if you send a transaction."));
        }
        walletInstance->m_pay_tx_fee = CFeeRate{pay_tx_fee.value(), 1000};
        if (chain && walletInstance->m_pay_tx_fee < chain->relayMinFee()) {
            error = strprintf(_("Invalid amount for -paytxfee=<amount>: '%s' (must be at least %s)"),
                args.GetArg("-paytxfee", ""), chain->relayMinFee().ToString());
            return nullptr;
        }
    }
    if (args.IsArgSet("-maxtxfee")) {
        std::optional<CAmount> max_fee = ParseMoney(args.GetArg("-maxtxfee", ""));
        if (!max_fee) {
            error = AmountErrMsg("maxtxfee", args.GetArg("-maxtxfee", ""));
            return nullptr;
        } else if (max_fee.value() > HIGH_MAX_TX_FEE) {
            warnings.push_back(_("-maxtxfee is set very high! Fees this large could be paid on a single transaction."));
        }
        if (chain && CFeeRate{max_fee.value(), 1000} < chain->relayMinFee()) {
            error = strprintf(_("Invalid amount for -maxtxfee=<amount>: '%s' (must be at least the minrelay fee of %s to prevent stuck transactions)"),
                args.GetArg("-maxtxfee", ""), chain->relayMinFee().ToString());
            return nullptr;
        }
        walletInstance->m_default_max_tx_fee = max_fee.value();
    }
    if (args.IsArgSet("-consolidatefeerate")) {
        if (std::optional<CAmount> consolidate_feerate = ParseMoney(args.GetArg("-consolidatefeerate", ""))) {
            walletInstance->m_consolidate_feerate = CFeeRate(*consolidate_feerate);
        } else {
            error = AmountErrMsg("consolidatefeerate", args.GetArg("-consolidatefeerate", ""));
            return nullptr;
        }
    }
    if (chain && chain->relayMinFee().GetFeePerK() > HIGH_TX_FEE_PER_KB) {
        warnings.push_back(AmountHighWarn("-minrelaytxfee") + Untranslated(" ") +
                           _("The wallet will avoid paying less than the minimum relay fee."));
    }
    walletInstance->m_confirm_target = args.GetIntArg("-txconfirmtarget", DEFAULT_TX_CONFIRM_TARGET);
    walletInstance->m_spend_zero_conf_change = args.GetBoolArg("-spendzeroconfchange", DEFAULT_SPEND_ZEROCONF_CHANGE);
    walletInstance->m_signal_rbf = args.GetBoolArg("-walletrbf", DEFAULT_WALLET_RBF);
    walletInstance->WalletLogPrintf("Wallet completed loading in %15dms\n", Ticks<std::chrono::milliseconds>(SteadyClock::now() - start));
    walletInstance->TopUpKeyPool();
    if (chain && !AttachChain(walletInstance, *chain, rescan_required, error, warnings)) {
        return nullptr;
    }
    {
        LOCK(walletInstance->cs_wallet);
        walletInstance->SetBroadcastTransactions(args.GetBoolArg("-walletbroadcast", DEFAULT_WALLETBROADCAST));
        walletInstance->WalletLogPrintf("setKeyPool.size() = %u\n",      walletInstance->GetKeyPoolSize());
        walletInstance->WalletLogPrintf("mapWallet.size() = %u\n",       walletInstance->mapWallet.size());
        walletInstance->WalletLogPrintf("m_address_book.size() = %u\n",  walletInstance->m_address_book.size());
    }
    return walletInstance;
}
bool CWallet::AttachChain(const std::shared_ptr<CWallet>& walletInstance, interfaces::Chain& chain, const bool rescan_required, bilingual_str& error, std::vector<bilingual_str>& warnings)
{
    LOCK(walletInstance->cs_wallet);
    assert(!walletInstance->m_chain || walletInstance->m_chain == &chain);
    walletInstance->m_chain = &chain;
    if (!gArgs.GetBoolArg("-walletcrosschain", DEFAULT_WALLETCROSSCHAIN)) {
        WalletBatch batch(walletInstance->GetDatabase());
        CBlockLocator locator;
        if (batch.ReadBestBlock(locator) && locator.vHave.size() > 0 && chain.getHeight()) {
            if (chain.getBlockHash(0) != locator.vHave.back()) {
                error = Untranslated("Wallet files should not be reused across chains. Restart lcrypd with -walletcrosschain to override.");
                return false;
            }
        }
    }
    walletInstance->m_attaching_chain = true;
    walletInstance->m_chain_notifications_handler = walletInstance->chain().handleNotifications(walletInstance);
    int rescan_height = 0;
    if (!rescan_required)
    {
        WalletBatch batch(walletInstance->GetDatabase());
        CBlockLocator locator;
        if (batch.ReadBestBlock(locator)) {
            if (const std::optional<int> fork_height = chain.findLocatorFork(locator)) {
                rescan_height = *fork_height;
            }
        }
    }
    const std::optional<int> tip_height = chain.getHeight();
    if (tip_height) {
        walletInstance->m_last_block_processed = chain.getBlockHash(*tip_height);
        walletInstance->m_last_block_processed_height = *tip_height;
    } else {
        walletInstance->m_last_block_processed.SetNull();
        walletInstance->m_last_block_processed_height = -1;
    }
    if (tip_height && *tip_height != rescan_height)
    {
        if (chain.havePruned() || chain.hasAssumedValidChain()) {
            int block_height = *tip_height;
            while (block_height > 0 && chain.haveBlockOnDisk(block_height - 1) && rescan_height != block_height) {
                --block_height;
            }
            if (rescan_height != block_height) {
                error = chain.hasAssumedValidChain() ?
                     _(
                        "Assumed-valid: last wallet synchronisation goes beyond "
                        "available block data. You need to wait for the background "
                        "validation chain to download more blocks.") :
                     _("Prune: last wallet synchronisation goes beyond pruned data. You need to -reindex (download the whole blockchain again in case of pruned node)");
                return false;
            }
        }
        chain.initMessage(_("Rescanning…").translated);
        walletInstance->WalletLogPrintf("Rescanning last %i blocks (from block %i)...\n", *tip_height - rescan_height, rescan_height);
        std::optional<int64_t> time_first_key;
        for (auto spk_man : walletInstance->GetAllScriptPubKeyMans()) {
            int64_t time = spk_man->GetTimeFirstKey();
            if (!time_first_key || time < *time_first_key) time_first_key = time;
        }
        if (time_first_key) {
            chain.findFirstBlockWithTimeAndHeight(*time_first_key - TIMESTAMP_WINDOW, rescan_height, FoundBlock().height(rescan_height));
        }
        {
            WalletRescanReserver reserver(*walletInstance);
            if (!reserver.reserve() || (ScanResult::SUCCESS != walletInstance->ScanForWalletTransactions(chain.getBlockHash(rescan_height), rescan_height,  {}, reserver,  true,  true).status)) {
                error = _("Failed to rescan the wallet during initialization");
                return false;
            }
        }
        walletInstance->m_attaching_chain = false;
        walletInstance->chainStateFlushed(chain.getTipLocator());
        walletInstance->GetDatabase().IncrementUpdateCounter();
    }
    walletInstance->m_attaching_chain = false;
    return true;
}
const CAddressBookData* CWallet::FindAddressBookEntry(const CTxDestination& dest, bool allow_change) const
{
    const auto& address_book_it = m_address_book.find(dest);
    if (address_book_it == m_address_book.end()) return nullptr;
    if ((!allow_change) && address_book_it->second.IsChange()) {
        return nullptr;
    }
    return &address_book_it->second;
}
bool CWallet::UpgradeWallet(int version, bilingual_str& error)
{
    int prev_version = GetVersion();
    if (version == 0) {
        WalletLogPrintf("Performing wallet upgrade to %i\n", FEATURE_LATEST);
        version = FEATURE_LATEST;
    } else {
        WalletLogPrintf("Allowing wallet upgrade up to %i\n", version);
    }
    if (version < prev_version) {
        error = strprintf(_("Cannot downgrade wallet from version %i to version %i. Wallet version unchanged."), prev_version, version);
        return false;
    }
    LOCK(cs_wallet);
    if (!CanSupportFeature(FEATURE_HD_SPLIT) && version >= FEATURE_HD_SPLIT && version < FEATURE_PRE_SPLIT_KEYPOOL) {
        error = strprintf(_("Cannot upgrade a non HD split wallet from version %i to version %i without upgrading to support pre-split keypool. Please use version %i or no version specified."), prev_version, version, FEATURE_PRE_SPLIT_KEYPOOL);
        return false;
    }
    SetMinVersion(GetClosestWalletFeature(version));
    for (auto spk_man : GetActiveScriptPubKeyMans()) {
        if (!spk_man->Upgrade(prev_version, version, error)) {
            return false;
        }
    }
    return true;
}
void CWallet::postInitProcess()
{
    LOCK(cs_wallet);
    ResubmitWalletTransactions( false,  true);
    chain().requestMempoolTransactions(*this);
}
bool CWallet::BackupWallet(const std::string& strDest) const
{
    return GetDatabase().Backup(strDest);
}
CKeyPool::CKeyPool()
{
    nTime = GetTime();
    fInternal = false;
    m_pre_split = false;
}
CKeyPool::CKeyPool(const CPubKey& vchPubKeyIn, bool internalIn)
{
    nTime = GetTime();
    vchPubKey = vchPubKeyIn;
    fInternal = internalIn;
    m_pre_split = false;
}
int CWallet::GetTxDepthInMainChain(const CWalletTx& wtx) const
{
    AssertLockHeld(cs_wallet);
    if (auto* conf = wtx.state<TxStateConfirmed>()) {
        return GetLastBlockHeight() - conf->confirmed_block_height + 1;
    } else if (auto* conf = wtx.state<TxStateConflicted>()) {
        return -1 * (GetLastBlockHeight() - conf->conflicting_block_height + 1);
    } else {
        return 0;
    }
}
int CWallet::GetTxBlocksToMaturity(const CWalletTx& wtx) const
{
    AssertLockHeld(cs_wallet);
    if (!wtx.IsCoinBase()) {
        return 0;
    }
    int chain_depth = GetTxDepthInMainChain(wtx);
    assert(chain_depth >= 0);
    return std::max(0, (COINBASE_MATURITY+1) - chain_depth);
}
bool CWallet::IsTxImmatureCoinBase(const CWalletTx& wtx) const
{
    AssertLockHeld(cs_wallet);
    return GetTxBlocksToMaturity(wtx) > 0;
}
bool CWallet::IsCrypted() const
{
    return HasEncryptionKeys();
}
bool CWallet::IsLocked() const
{
    if (!IsCrypted()) {
        return false;
    }
    LOCK(cs_wallet);
    return vMasterKey.empty();
}
bool CWallet::Lock()
{
    if (!IsCrypted())
        return false;
    {
        LOCK(cs_wallet);
        vMasterKey.clear();
    }
    NotifyStatusChanged(this);
    return true;
}
bool CWallet::Unlock(const CKeyingMaterial& vMasterKeyIn, bool accept_no_keys)
{
    {
        LOCK(cs_wallet);
        for (const auto& spk_man_pair : m_spk_managers) {
            if (!spk_man_pair.second->CheckDecryptionKey(vMasterKeyIn, accept_no_keys)) {
                return false;
            }
        }
        vMasterKey = vMasterKeyIn;
    }
    NotifyStatusChanged(this);
    return true;
}
std::set<ScriptPubKeyMan*> CWallet::GetActiveScriptPubKeyMans() const
{
    std::set<ScriptPubKeyMan*> spk_mans;
    for (bool internal : {false, true}) {
        for (OutputType t : OUTPUT_TYPES) {
            auto spk_man = GetScriptPubKeyMan(t, internal);
            if (spk_man) {
                spk_mans.insert(spk_man);
            }
        }
    }
    return spk_mans;
}
std::set<ScriptPubKeyMan*> CWallet::GetAllScriptPubKeyMans() const
{
    std::set<ScriptPubKeyMan*> spk_mans;
    for (const auto& spk_man_pair : m_spk_managers) {
        spk_mans.insert(spk_man_pair.second.get());
    }
    return spk_mans;
}
ScriptPubKeyMan* CWallet::GetScriptPubKeyMan(const OutputType& type, bool internal) const
{
    const std::map<OutputType, ScriptPubKeyMan*>& spk_managers = internal ? m_internal_spk_managers : m_external_spk_managers;
    std::map<OutputType, ScriptPubKeyMan*>::const_iterator it = spk_managers.find(type);
    if (it == spk_managers.end()) {
        return nullptr;
    }
    return it->second;
}
std::set<ScriptPubKeyMan*> CWallet::GetScriptPubKeyMans(const CScript& script) const
{
    std::set<ScriptPubKeyMan*> spk_mans;
    SignatureData sigdata;
    for (const auto& spk_man_pair : m_spk_managers) {
        if (spk_man_pair.second->CanProvide(script, sigdata)) {
            spk_mans.insert(spk_man_pair.second.get());
        }
    }
    return spk_mans;
}
ScriptPubKeyMan* CWallet::GetScriptPubKeyMan(const uint256& id) const
{
    if (m_spk_managers.count(id) > 0) {
        return m_spk_managers.at(id).get();
    }
    return nullptr;
}
std::unique_ptr<SigningProvider> CWallet::GetSolvingProvider(const CScript& script) const
{
    SignatureData sigdata;
    return GetSolvingProvider(script, sigdata);
}
std::unique_ptr<SigningProvider> CWallet::GetSolvingProvider(const CScript& script, SignatureData& sigdata) const
{
    for (const auto& spk_man_pair : m_spk_managers) {
        if (spk_man_pair.second->CanProvide(script, sigdata)) {
            return spk_man_pair.second->GetSolvingProvider(script);
        }
    }
    return nullptr;
}
std::vector<WalletDescriptor> CWallet::GetWalletDescriptors(const CScript& script) const
{
    std::vector<WalletDescriptor> descs;
    for (const auto spk_man: GetScriptPubKeyMans(script)) {
        if (const auto desc_spk_man = dynamic_cast<DescriptorScriptPubKeyMan*>(spk_man)) {
            LOCK(desc_spk_man->cs_desc_man);
            descs.push_back(desc_spk_man->GetWalletDescriptor());
        }
    }
    return descs;
}
LegacyScriptPubKeyMan* CWallet::GetLegacyScriptPubKeyMan() const
{
    if (IsWalletFlagSet(WALLET_FLAG_DESCRIPTORS)) {
        return nullptr;
    }
    auto it = m_internal_spk_managers.find(OutputType::LEGACY);
    if (it == m_internal_spk_managers.end()) return nullptr;
    return dynamic_cast<LegacyScriptPubKeyMan*>(it->second);
}
LegacyScriptPubKeyMan* CWallet::GetOrCreateLegacyScriptPubKeyMan()
{
    SetupLegacyScriptPubKeyMan();
    return GetLegacyScriptPubKeyMan();
}
void CWallet::SetupLegacyScriptPubKeyMan()
{
    if (!m_internal_spk_managers.empty() || !m_external_spk_managers.empty() || !m_spk_managers.empty() || IsWalletFlagSet(WALLET_FLAG_DESCRIPTORS)) {
        return;
    }
    auto spk_manager = std::unique_ptr<ScriptPubKeyMan>(new LegacyScriptPubKeyMan(*this));
    for (const auto& type : LEGACY_OUTPUT_TYPES) {
        m_internal_spk_managers[type] = spk_manager.get();
        m_external_spk_managers[type] = spk_manager.get();
    }
    m_spk_managers[spk_manager->GetID()] = std::move(spk_manager);
}
const CKeyingMaterial& CWallet::GetEncryptionKey() const
{
    return vMasterKey;
}
bool CWallet::HasEncryptionKeys() const
{
    return !mapMasterKeys.empty();
}
void CWallet::ConnectScriptPubKeyManNotifiers()
{
    for (const auto& spk_man : GetActiveScriptPubKeyMans()) {
        spk_man->NotifyWatchonlyChanged.connect(NotifyWatchonlyChanged);
        spk_man->NotifyCanGetAddressesChanged.connect(NotifyCanGetAddressesChanged);
    }
}
void CWallet::LoadDescriptorScriptPubKeyMan(uint256 id, WalletDescriptor& desc)
{
    if (IsWalletFlagSet(WALLET_FLAG_EXTERNAL_SIGNER)) {
        auto spk_manager = std::unique_ptr<ScriptPubKeyMan>(new ExternalSignerScriptPubKeyMan(*this, desc));
        m_spk_managers[id] = std::move(spk_manager);
    } else {
        auto spk_manager = std::unique_ptr<ScriptPubKeyMan>(new DescriptorScriptPubKeyMan(*this, desc));
        m_spk_managers[id] = std::move(spk_manager);
    }
}
void CWallet::SetupDescriptorScriptPubKeyMans(const CExtKey& master_key)
{
    AssertLockHeld(cs_wallet);
    for (bool internal : {false, true}) {
        for (OutputType t : OUTPUT_TYPES) {
            auto spk_manager = std::unique_ptr<DescriptorScriptPubKeyMan>(new DescriptorScriptPubKeyMan(*this));
            if (IsCrypted()) {
                if (IsLocked()) {
                    throw std::runtime_error(std::string(__func__) + ": Wallet is locked, cannot setup new descriptors");
                }
                if (!spk_manager->CheckDecryptionKey(vMasterKey) && !spk_manager->Encrypt(vMasterKey, nullptr)) {
                    throw std::runtime_error(std::string(__func__) + ": Could not encrypt new descriptors");
                }
            }
            spk_manager->SetupDescriptorGeneration(master_key, t, internal);
            uint256 id = spk_manager->GetID();
            m_spk_managers[id] = std::move(spk_manager);
            AddActiveScriptPubKeyMan(id, t, internal);
        }
    }
}
void CWallet::SetupDescriptorScriptPubKeyMans()
{
    AssertLockHeld(cs_wallet);
    if (!IsWalletFlagSet(WALLET_FLAG_EXTERNAL_SIGNER)) {
        CKey seed_key;
        seed_key.MakeNewKey(true);
        CPubKey seed = seed_key.GetPubKey();
        assert(seed_key.VerifyPubKey(seed));
        CExtKey master_key;
        master_key.SetSeed(seed_key);
        SetupDescriptorScriptPubKeyMans(master_key);
    } else {
        ExternalSigner signer = ExternalSignerScriptPubKeyMan::GetExternalSigner();
        int account = 0;
        UniValue signer_res = signer.GetDescriptors(account);
        if (!signer_res.isObject()) throw std::runtime_error(std::string(__func__) + ": Unexpected result");
        for (bool internal : {false, true}) {
            const UniValue& descriptor_vals = find_value(signer_res, internal ? "internal" : "receive");
            if (!descriptor_vals.isArray()) throw std::runtime_error(std::string(__func__) + ": Unexpected result");
            for (const UniValue& desc_val : descriptor_vals.get_array().getValues()) {
                const std::string& desc_str = desc_val.getValStr();
                FlatSigningProvider keys;
                std::string desc_error;
                std::unique_ptr<Descriptor> desc = Parse(desc_str, keys, desc_error, false);
                if (desc == nullptr) {
                    throw std::runtime_error(std::string(__func__) + ": Invalid descriptor \"" + desc_str + "\" (" + desc_error + ")");
                }
                if (!desc->GetOutputType()) {
                    continue;
                }
                OutputType t =  *desc->GetOutputType();
                auto spk_manager = std::unique_ptr<ExternalSignerScriptPubKeyMan>(new ExternalSignerScriptPubKeyMan(*this));
                spk_manager->SetupDescriptor(std::move(desc));
                uint256 id = spk_manager->GetID();
                m_spk_managers[id] = std::move(spk_manager);
                AddActiveScriptPubKeyMan(id, t, internal);
            }
        }
    }
}
void CWallet::AddActiveScriptPubKeyMan(uint256 id, OutputType type, bool internal)
{
    WalletBatch batch(GetDatabase());
    if (!batch.WriteActiveScriptPubKeyMan(static_cast<uint8_t>(type), id, internal)) {
        throw std::runtime_error(std::string(__func__) + ": writing active ScriptPubKeyMan id failed");
    }
    LoadActiveScriptPubKeyMan(id, type, internal);
}
void CWallet::LoadActiveScriptPubKeyMan(uint256 id, OutputType type, bool internal)
{
    Assert(IsWalletFlagSet(WALLET_FLAG_DESCRIPTORS));
    WalletLogPrintf("Setting spkMan to active: id = %s, type = %s, internal = %s\n", id.ToString(), FormatOutputType(type), internal ? "true" : "false");
    auto& spk_mans = internal ? m_internal_spk_managers : m_external_spk_managers;
    auto& spk_mans_other = internal ? m_external_spk_managers : m_internal_spk_managers;
    auto spk_man = m_spk_managers.at(id).get();
    spk_mans[type] = spk_man;
    const auto it = spk_mans_other.find(type);
    if (it != spk_mans_other.end() && it->second == spk_man) {
        spk_mans_other.erase(type);
    }
    NotifyCanGetAddressesChanged();
}
void CWallet::DeactivateScriptPubKeyMan(uint256 id, OutputType type, bool internal)
{
    auto spk_man = GetScriptPubKeyMan(type, internal);
    if (spk_man != nullptr && spk_man->GetID() == id) {
        WalletLogPrintf("Deactivate spkMan: id = %s, type = %s, internal = %s\n", id.ToString(), FormatOutputType(type), internal ? "true" : "false");
        WalletBatch batch(GetDatabase());
        if (!batch.EraseActiveScriptPubKeyMan(static_cast<uint8_t>(type), internal)) {
            throw std::runtime_error(std::string(__func__) + ": erasing active ScriptPubKeyMan id failed");
        }
        auto& spk_mans = internal ? m_internal_spk_managers : m_external_spk_managers;
        spk_mans.erase(type);
    }
    NotifyCanGetAddressesChanged();
}
bool CWallet::IsLegacy() const
{
    if (m_internal_spk_managers.count(OutputType::LEGACY) == 0) {
        return false;
    }
    auto spk_man = dynamic_cast<LegacyScriptPubKeyMan*>(m_internal_spk_managers.at(OutputType::LEGACY));
    return spk_man != nullptr;
}
DescriptorScriptPubKeyMan* CWallet::GetDescriptorScriptPubKeyMan(const WalletDescriptor& desc) const
{
    for (auto& spk_man_pair : m_spk_managers) {
        DescriptorScriptPubKeyMan* spk_manager = dynamic_cast<DescriptorScriptPubKeyMan*>(spk_man_pair.second.get());
        if (spk_manager != nullptr && spk_manager->HasWalletDescriptor(desc)) {
            return spk_manager;
        }
    }
    return nullptr;
}
std::optional<bool> CWallet::IsInternalScriptPubKeyMan(ScriptPubKeyMan* spk_man) const
{
    if (IsLegacy()) {
        return std::nullopt;
    }
    if (!GetActiveScriptPubKeyMans().count(spk_man)) {
        return std::nullopt;
    }
    const auto desc_spk_man = dynamic_cast<DescriptorScriptPubKeyMan*>(spk_man);
    if (!desc_spk_man) {
        throw std::runtime_error(std::string(__func__) + ": unexpected ScriptPubKeyMan type.");
    }
    LOCK(desc_spk_man->cs_desc_man);
    const auto& type = desc_spk_man->GetWalletDescriptor().descriptor->GetOutputType();
    assert(type.has_value());
    return GetScriptPubKeyMan(*type,   true) == desc_spk_man;
}
ScriptPubKeyMan* CWallet::AddWalletDescriptor(WalletDescriptor& desc, const FlatSigningProvider& signing_provider, const std::string& label, bool internal)
{
    AssertLockHeld(cs_wallet);
    if (!IsWalletFlagSet(WALLET_FLAG_DESCRIPTORS)) {
        WalletLogPrintf("Cannot add WalletDescriptor to a non-descriptor wallet\n");
        return nullptr;
    }
    auto spk_man = GetDescriptorScriptPubKeyMan(desc);
    if (spk_man) {
        WalletLogPrintf("Update existing descriptor: %s\n", desc.descriptor->ToString());
        spk_man->UpdateWalletDescriptor(desc);
    } else {
        auto new_spk_man = std::unique_ptr<DescriptorScriptPubKeyMan>(new DescriptorScriptPubKeyMan(*this, desc));
        spk_man = new_spk_man.get();
        m_spk_managers[new_spk_man->GetID()] = std::move(new_spk_man);
    }
    for (const auto& entry : signing_provider.keys) {
        const CKey& key = entry.second;
        spk_man->AddDescriptorKey(key, key.GetPubKey());
    }
    if (!spk_man->TopUp()) {
        WalletLogPrintf("Could not top up scriptPubKeys\n");
        return nullptr;
    }
    if (!desc.descriptor->IsRange()) {
        auto script_pub_keys = spk_man->GetScriptPubKeys();
        if (script_pub_keys.empty()) {
            WalletLogPrintf("Could not generate scriptPubKeys (cache is empty)\n");
            return nullptr;
        }
        if (!internal) {
            for (const auto& script : script_pub_keys) {
                CTxDestination dest;
                if (ExtractDestination(script, dest)) {
                    SetAddressBook(dest, label, "receive");
                }
            }
        }
    }
    spk_man->WriteDescriptor();
    return spk_man;
}
bool CWallet::MigrateToSQLite(bilingual_str& error)
{
    AssertLockHeld(cs_wallet);
    WalletLogPrintf("Migrating wallet storage database from BerkeleyDB to SQLite.\n");
    if (m_database->Format() == "sqlite") {
        error = _("Error: This wallet already uses SQLite");
        return false;
    }
    std::unique_ptr<DatabaseBatch> batch = m_database->MakeBatch();
    std::vector<std::pair<SerializeData, SerializeData>> records;
    if (!batch->StartCursor()) {
        error = _("Error: Unable to begin reading all records in the database");
        return false;
    }
    bool complete = false;
    while (true) {
        CDataStream ss_key(SER_DISK, CLIENT_VERSION);
        CDataStream ss_value(SER_DISK, CLIENT_VERSION);
        bool ret = batch->ReadAtCursor(ss_key, ss_value, complete);
        if (!ret) {
            break;
        }
        SerializeData key(ss_key.begin(), ss_key.end());
        SerializeData value(ss_value.begin(), ss_value.end());
        records.emplace_back(key, value);
    }
    batch->CloseCursor();
    batch.reset();
    if (!complete) {
        error = _("Error: Unable to read all records in the database");
        return false;
    }
    fs::path db_path = fs::PathFromString(m_database->Filename());
    fs::path db_dir = db_path.parent_path();
    m_database->Close();
    fs::remove(db_path);
    DatabaseOptions opts;
    opts.require_create = true;
    opts.require_format = DatabaseFormat::SQLITE;
    DatabaseStatus db_status;
    std::unique_ptr<WalletDatabase> new_db = MakeDatabase(db_dir, opts, db_status, error);
    assert(new_db);
    m_database.reset();
    m_database = std::move(new_db);
    batch = m_database->MakeBatch();
    bool began = batch->TxnBegin();
    assert(began);
    for (const auto& [key, value] : records) {
        CDataStream ss_key(key, SER_DISK, CLIENT_VERSION);
        CDataStream ss_value(value, SER_DISK, CLIENT_VERSION);
        if (!batch->Write(ss_key, ss_value)) {
            batch->TxnAbort();
            m_database->Close();
            fs::remove(m_database->Filename());
            assert(false);
        }
    }
    bool committed = batch->TxnCommit();
    assert(committed);
    return true;
}
std::optional<MigrationData> CWallet::GetDescriptorsForLegacy(bilingual_str& error) const
{
    AssertLockHeld(cs_wallet);
    LegacyScriptPubKeyMan* legacy_spkm = GetLegacyScriptPubKeyMan();
    if (!legacy_spkm) {
        error = _("Error: This wallet is already a descriptor wallet");
        return std::nullopt;
    }
    std::optional<MigrationData> res = legacy_spkm->MigrateToDescriptor();
    if (res == std::nullopt) {
        error = _("Error: Unable to produce descriptors for this legacy wallet. Make sure the wallet is unlocked first");
        return std::nullopt;
    }
    return res;
}
bool CWallet::ApplyMigrationData(MigrationData& data, bilingual_str& error)
{
    AssertLockHeld(cs_wallet);
    LegacyScriptPubKeyMan* legacy_spkm = GetLegacyScriptPubKeyMan();
    if (!legacy_spkm) {
        error = _("Error: This wallet is already a descriptor wallet");
        return false;
    }
    for (auto& desc_spkm : data.desc_spkms) {
        if (m_spk_managers.count(desc_spkm->GetID()) > 0) {
            error = _("Error: Duplicate descriptors created during migration. Your wallet may be corrupted.");
            return false;
        }
        m_spk_managers[desc_spkm->GetID()] = std::move(desc_spkm);
    }
    if (!legacy_spkm->DeleteRecords()) {
        return false;
    }
    m_spk_managers.erase(legacy_spkm->GetID());
    m_external_spk_managers.clear();
    m_internal_spk_managers.clear();
    SetWalletFlag(WALLET_FLAG_DESCRIPTORS);
    if (!IsWalletFlagSet(WALLET_FLAG_DISABLE_PRIVATE_KEYS)) {
        if (data.master_key.key.IsValid()) {
            SetupDescriptorScriptPubKeyMans(data.master_key);
        } else {
            SetupDescriptorScriptPubKeyMans();
        }
    }
    std::vector<uint256> txids_to_delete;
    for (const auto& [_pos, wtx] : wtxOrdered) {
        if (!IsMine(*wtx->tx) && !IsFromMe(*wtx->tx)) {
            if (data.watchonly_wallet) {
                LOCK(data.watchonly_wallet->cs_wallet);
                if (data.watchonly_wallet->IsMine(*wtx->tx) || data.watchonly_wallet->IsFromMe(*wtx->tx)) {
                    if (!data.watchonly_wallet->AddToWallet(wtx->tx, wtx->m_state)) {
                        error = _("Error: Could not add watchonly tx to watchonly wallet");
                        return false;
                    }
                    txids_to_delete.push_back(wtx->GetHash());
                    continue;
                }
            }
            error = strprintf(_("Error: Transaction %s in wallet cannot be identified to belong to migrated wallets"), wtx->GetHash().GetHex());
            return false;
        }
    }
    if (txids_to_delete.size() > 0) {
        std::vector<uint256> deleted_txids;
        if (ZapSelectTx(txids_to_delete, deleted_txids) != DBErrors::LOAD_OK) {
            error = _("Error: Could not delete watchonly transactions");
            return false;
        }
        if (deleted_txids != txids_to_delete) {
            error = _("Error: Not all watchonly txs could be deleted");
            return false;
        }
        for (const uint256& txid : deleted_txids) {
            NotifyTransactionChanged(txid, CT_UPDATED);
        }
    }
    std::vector<CTxDestination> dests_to_delete;
    for (const auto& addr_pair : m_address_book) {
        if (addr_pair.second.purpose == "receive") {
            if (!IsMine(addr_pair.first)) {
                if (data.watchonly_wallet) {
                    LOCK(data.watchonly_wallet->cs_wallet);
                    if (data.watchonly_wallet->IsMine(addr_pair.first)) {
                        std::string label = addr_pair.second.GetLabel();
                        std::string purpose = addr_pair.second.purpose;
                        if (!purpose.empty()) {
                            data.watchonly_wallet->m_address_book[addr_pair.first].purpose = purpose;
                        }
                        if (!addr_pair.second.IsChange()) {
                            data.watchonly_wallet->m_address_book[addr_pair.first].SetLabel(label);
                        }
                        dests_to_delete.push_back(addr_pair.first);
                        continue;
                    }
                }
                if (data.solvable_wallet) {
                    LOCK(data.solvable_wallet->cs_wallet);
                    if (data.solvable_wallet->IsMine(addr_pair.first)) {
                        std::string label = addr_pair.second.GetLabel();
                        std::string purpose = addr_pair.second.purpose;
                        if (!purpose.empty()) {
                            data.solvable_wallet->m_address_book[addr_pair.first].purpose = purpose;
                        }
                        if (!addr_pair.second.IsChange()) {
                            data.solvable_wallet->m_address_book[addr_pair.first].SetLabel(label);
                        }
                        dests_to_delete.push_back(addr_pair.first);
                        continue;
                    }
                }
                error = _("Error: Address book data in wallet cannot be identified to belong to migrated wallets");
                return false;
            }
        } else {
            if (data.watchonly_wallet) {
                LOCK(data.watchonly_wallet->cs_wallet);
                std::string label = addr_pair.second.GetLabel();
                std::string purpose = addr_pair.second.purpose;
                if (!purpose.empty()) {
                    data.watchonly_wallet->m_address_book[addr_pair.first].purpose = purpose;
                }
                if (!addr_pair.second.IsChange()) {
                    data.watchonly_wallet->m_address_book[addr_pair.first].SetLabel(label);
                }
                continue;
            }
            if (data.solvable_wallet) {
                LOCK(data.solvable_wallet->cs_wallet);
                std::string label = addr_pair.second.GetLabel();
                std::string purpose = addr_pair.second.purpose;
                if (!purpose.empty()) {
                    data.solvable_wallet->m_address_book[addr_pair.first].purpose = purpose;
                }
                if (!addr_pair.second.IsChange()) {
                    data.solvable_wallet->m_address_book[addr_pair.first].SetLabel(label);
                }
                continue;
            }
        }
    }
    if (dests_to_delete.size() > 0) {
        for (const auto& dest : dests_to_delete) {
            if (!DelAddressBook(dest)) {
                error = _("Error: Unable to remove watchonly address book data");
                return false;
            }
        }
    }
    ConnectScriptPubKeyManNotifiers();
    NotifyCanGetAddressesChanged();
    WalletLogPrintf("Wallet migration complete.\n");
    return true;
}
bool DoMigration(CWallet& wallet, WalletContext& context, bilingual_str& error, MigrationResult& res) EXCLUSIVE_LOCKS_REQUIRED(wallet.cs_wallet)
{
    AssertLockHeld(wallet.cs_wallet);
    std::optional<MigrationData> data = wallet.GetDescriptorsForLegacy(error);
    if (data == std::nullopt) return false;
    if (data->watch_descs.size() > 0 || data->solvable_descs.size() > 0) {
        DatabaseOptions options;
        options.require_existing = false;
        options.require_create = true;
        options.create_flags = WALLET_FLAG_DISABLE_PRIVATE_KEYS | WALLET_FLAG_BLANK_WALLET | WALLET_FLAG_DESCRIPTORS;
        if (wallet.IsWalletFlagSet(WALLET_FLAG_AVOID_REUSE)) {
            options.create_flags |= WALLET_FLAG_AVOID_REUSE;
        }
        if (wallet.IsWalletFlagSet(WALLET_FLAG_KEY_ORIGIN_METADATA)) {
            options.create_flags |= WALLET_FLAG_KEY_ORIGIN_METADATA;
        }
        if (data->watch_descs.size() > 0) {
            wallet.WalletLogPrintf("Making a new watchonly wallet containing the watched scripts\n");
            DatabaseStatus status;
            std::vector<bilingual_str> warnings;
            std::string wallet_name = wallet.GetName() + "_watchonly";
            data->watchonly_wallet = CreateWallet(context, wallet_name, std::nullopt, options, status, error, warnings);
            if (status != DatabaseStatus::SUCCESS) {
                error = _("Error: Failed to create new watchonly wallet");
                return false;
            }
            res.watchonly_wallet = data->watchonly_wallet;
            LOCK(data->watchonly_wallet->cs_wallet);
            for (const auto& [desc_str, creation_time] : data->watch_descs) {
                FlatSigningProvider keys;
                std::string parse_err;
                std::unique_ptr<Descriptor> desc = Parse(desc_str, keys, parse_err,   true);
                assert(desc);
                assert(!desc->IsRange());
                WalletDescriptor w_desc(std::move(desc), creation_time, 0, 0, 0);
                data->watchonly_wallet->AddWalletDescriptor(w_desc, keys, "", false);
            }
            UpdateWalletSetting(*context.chain, wallet_name,  true, warnings);
        }
        if (data->solvable_descs.size() > 0) {
            wallet.WalletLogPrintf("Making a new watchonly wallet containing the unwatched solvable scripts\n");
            DatabaseStatus status;
            std::vector<bilingual_str> warnings;
            std::string wallet_name = wallet.GetName() + "_solvables";
            data->solvable_wallet = CreateWallet(context, wallet_name, std::nullopt, options, status, error, warnings);
            if (status != DatabaseStatus::SUCCESS) {
                error = _("Error: Failed to create new watchonly wallet");
                return false;
            }
            res.solvables_wallet = data->solvable_wallet;
            LOCK(data->solvable_wallet->cs_wallet);
            for (const auto& [desc_str, creation_time] : data->solvable_descs) {
                FlatSigningProvider keys;
                std::string parse_err;
                std::unique_ptr<Descriptor> desc = Parse(desc_str, keys, parse_err,   true);
                assert(desc);
                assert(!desc->IsRange());
                WalletDescriptor w_desc(std::move(desc), creation_time, 0, 0, 0);
                data->solvable_wallet->AddWalletDescriptor(w_desc, keys, "", false);
            }
            UpdateWalletSetting(*context.chain, wallet_name,  true, warnings);
        }
    }
    if (!wallet.ApplyMigrationData(*data, error)) {
        return false;
    }
    return true;
}
util::Result<MigrationResult> MigrateLegacyToDescriptor(std::shared_ptr<CWallet>&& wallet, WalletContext& context)
{
    MigrationResult res;
    bilingual_str error;
    std::vector<bilingual_str> warnings;
    std::string wallet_name = wallet->GetName();
    fs::path this_wallet_dir = fs::absolute(fs::PathFromString(wallet->GetDatabase().Filename())).parent_path();
    fs::path backup_filename = fs::PathFromString(strprintf("%s-%d.legacy.bak", wallet_name, GetTime()));
    fs::path backup_path = this_wallet_dir / backup_filename;
    if (!wallet->BackupWallet(fs::PathToString(backup_path))) {
        return util::Error{_("Error: Unable to make a backup of your wallet")};
    }
    res.backup_path = backup_path;
    if (!RemoveWallet(context, wallet,  std::nullopt, warnings)) {
        return util::Error{_("Unable to unload the wallet before migrating")};
    }
    UnloadWallet(std::move(wallet));
    WalletContext empty_context;
    empty_context.args = context.args;
    DatabaseOptions options;
    options.require_existing = true;
    DatabaseStatus status;
    std::unique_ptr<WalletDatabase> database = MakeWalletDatabase(wallet_name, options, status, error);
    if (!database) {
        return util::Error{Untranslated("Wallet file verification failed.") + Untranslated(" ") + error};
    }
    std::shared_ptr<CWallet> local_wallet = CWallet::Create(empty_context, wallet_name, std::move(database), options.create_flags, error, warnings);
    if (!local_wallet) {
        return util::Error{Untranslated("Wallet loading failed.") + Untranslated(" ") + error};
    }
    bool success = false;
    {
        LOCK(local_wallet->cs_wallet);
        if (!local_wallet->MigrateToSQLite(error)) return util::Error{error};
        success = DoMigration(*local_wallet, context, error, res);
    }
    if (success) {
        assert(local_wallet.use_count() == 1);
        local_wallet.reset();
        LoadWallet(context, wallet_name,  std::nullopt, options, status, error, warnings);
        res.wallet_name = wallet_name;
    } else {
        fs::path temp_backup_location = fsbridge::AbsPathJoin(GetWalletDir(), backup_filename);
        fs::copy_file(backup_path, temp_backup_location, fs::copy_options::none);
        std::vector<fs::path> wallet_dirs;
        wallet_dirs.push_back(fs::PathFromString(local_wallet->GetDatabase().Filename()).parent_path());
        assert(local_wallet.use_count() == 1);
        local_wallet.reset();
        std::vector<std::shared_ptr<CWallet>> created_wallets;
        created_wallets.push_back(std::move(res.watchonly_wallet));
        created_wallets.push_back(std::move(res.solvables_wallet));
        for (std::shared_ptr<CWallet>& w : created_wallets) {
            wallet_dirs.push_back(fs::PathFromString(w->GetDatabase().Filename()).parent_path());
        }
        for (std::shared_ptr<CWallet>& w : created_wallets) {
            if (!RemoveWallet(context, w,  false)) {
                error += _("\nUnable to cleanup failed migration");
                return util::Error{error};
            }
            UnloadWallet(std::move(w));
        }
        for (fs::path& dir : wallet_dirs) {
            fs::remove_all(dir);
        }
        DatabaseStatus status;
        std::vector<bilingual_str> warnings;
        if (!RestoreWallet(context, temp_backup_location, wallet_name,  std::nullopt, status, error, warnings)) {
            error += _("\nUnable to restore backup of wallet.");
            return util::Error{error};
        }
        fs::copy_file(temp_backup_location, backup_path, fs::copy_options::none);
        fs::remove(temp_backup_location);
        return util::Error{error};
    }
    return res;
}
}
