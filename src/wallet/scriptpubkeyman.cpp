#include <key_io.h>
#include <logging.h>
#include <outputtype.h>
#include <script/descriptor.h>
#include <script/sign.h>
#include <util/bip32.h>
#include <util/strencodings.h>
#include <util/string.h>
#include <util/system.h>
#include <util/time.h>
#include <util/translation.h>
#include <wallet/scriptpubkeyman.h>
#include <optional>
namespace wallet {
const uint32_t BIP32_HARDENED_KEY_LIMIT = 0x80000000;
util::Result<CTxDestination> LegacyScriptPubKeyMan::GetNewDestination(const OutputType type)
{
    if (LEGACY_OUTPUT_TYPES.count(type) == 0) {
        return util::Error{_("Error: Legacy wallets only support the \"legacy\", \"p2sh-segwit\", and \"bech32\" address types")};
    }
    assert(type != OutputType::BECH32M);
    LOCK(cs_KeyStore);
    CPubKey new_key;
    if (!GetKeyFromPool(new_key, type)) {
        return util::Error{_("Error: Keypool ran out, please call keypoolrefill first")};
    }
    LearnRelatedScripts(new_key, type);
    return GetDestinationForKey(new_key, type);
}
typedef std::vector<unsigned char> valtype;
namespace {
enum class IsMineSigVersion
{
    TOP = 0,
    P2SH = 1,
    WITNESS_V0 = 2,
};
enum class IsMineResult
{
    NO = 0,
    WATCH_ONLY = 1,
    SPENDABLE = 2,
    INVALID = 3,
};
bool PermitsUncompressed(IsMineSigVersion sigversion)
{
    return sigversion == IsMineSigVersion::TOP || sigversion == IsMineSigVersion::P2SH;
}
bool HaveKeys(const std::vector<valtype>& pubkeys, const LegacyScriptPubKeyMan& keystore)
{
    for (const valtype& pubkey : pubkeys) {
        CKeyID keyID = CPubKey(pubkey).GetID();
        if (!keystore.HaveKey(keyID)) return false;
    }
    return true;
}
IsMineResult IsMineInner(const LegacyScriptPubKeyMan& keystore, const CScript& scriptPubKey, IsMineSigVersion sigversion, bool recurse_scripthash=true)
{
    IsMineResult ret = IsMineResult::NO;
    std::vector<valtype> vSolutions;
    TxoutType whichType = Solver(scriptPubKey, vSolutions);
    CKeyID keyID;
    switch (whichType) {
    case TxoutType::NONSTANDARD:
    case TxoutType::NULL_DATA:
    case TxoutType::WITNESS_UNKNOWN:
    case TxoutType::WITNESS_V1_TAPROOT:
        break;
    case TxoutType::PUBKEY:
        keyID = CPubKey(vSolutions[0]).GetID();
        if (!PermitsUncompressed(sigversion) && vSolutions[0].size() != 33) {
            return IsMineResult::INVALID;
        }
        if (keystore.HaveKey(keyID)) {
            ret = std::max(ret, IsMineResult::SPENDABLE);
        }
        break;
    case TxoutType::WITNESS_V0_KEYHASH:
    {
        if (sigversion == IsMineSigVersion::WITNESS_V0) {
            return IsMineResult::INVALID;
        }
        if (sigversion == IsMineSigVersion::TOP && !keystore.HaveCScript(CScriptID(CScript() << OP_0 << vSolutions[0]))) {
            break;
        }
        ret = std::max(ret, IsMineInner(keystore, GetScriptForDestination(PKHash(uint160(vSolutions[0]))), IsMineSigVersion::WITNESS_V0));
        break;
    }
    case TxoutType::PUBKEYHASH:
        keyID = CKeyID(uint160(vSolutions[0]));
        if (!PermitsUncompressed(sigversion)) {
            CPubKey pubkey;
            if (keystore.GetPubKey(keyID, pubkey) && !pubkey.IsCompressed()) {
                return IsMineResult::INVALID;
            }
        }
        if (keystore.HaveKey(keyID)) {
            ret = std::max(ret, IsMineResult::SPENDABLE);
        }
        break;
    case TxoutType::SCRIPTHASH:
    {
        if (sigversion != IsMineSigVersion::TOP) {
            return IsMineResult::INVALID;
        }
        CScriptID scriptID = CScriptID(uint160(vSolutions[0]));
        CScript subscript;
        if (keystore.GetCScript(scriptID, subscript)) {
            ret = std::max(ret, recurse_scripthash ? IsMineInner(keystore, subscript, IsMineSigVersion::P2SH) : IsMineResult::SPENDABLE);
        }
        break;
    }
    case TxoutType::WITNESS_V0_SCRIPTHASH:
    {
        if (sigversion == IsMineSigVersion::WITNESS_V0) {
            return IsMineResult::INVALID;
        }
        if (sigversion == IsMineSigVersion::TOP && !keystore.HaveCScript(CScriptID(CScript() << OP_0 << vSolutions[0]))) {
            break;
        }
        uint160 hash;
        CRIPEMD160().Write(vSolutions[0].data(), vSolutions[0].size()).Finalize(hash.begin());
        CScriptID scriptID = CScriptID(hash);
        CScript subscript;
        if (keystore.GetCScript(scriptID, subscript)) {
            ret = std::max(ret, recurse_scripthash ? IsMineInner(keystore, subscript, IsMineSigVersion::WITNESS_V0) : IsMineResult::SPENDABLE);
        }
        break;
    }
    case TxoutType::MULTISIG:
    {
        if (sigversion == IsMineSigVersion::TOP) {
            break;
        }
        std::vector<valtype> keys(vSolutions.begin()+1, vSolutions.begin()+vSolutions.size()-1);
        if (!PermitsUncompressed(sigversion)) {
            for (size_t i = 0; i < keys.size(); i++) {
                if (keys[i].size() != 33) {
                    return IsMineResult::INVALID;
                }
            }
        }
        if (HaveKeys(keys, keystore)) {
            ret = std::max(ret, IsMineResult::SPENDABLE);
        }
        break;
    }
    }
    if (ret == IsMineResult::NO && keystore.HaveWatchOnly(scriptPubKey)) {
        ret = std::max(ret, IsMineResult::WATCH_ONLY);
    }
    return ret;
}
}
isminetype LegacyScriptPubKeyMan::IsMine(const CScript& script) const
{
    switch (IsMineInner(*this, script, IsMineSigVersion::TOP)) {
    case IsMineResult::INVALID:
    case IsMineResult::NO:
        return ISMINE_NO;
    case IsMineResult::WATCH_ONLY:
        return ISMINE_WATCH_ONLY;
    case IsMineResult::SPENDABLE:
        return ISMINE_SPENDABLE;
    }
    assert(false);
}
bool LegacyScriptPubKeyMan::CheckDecryptionKey(const CKeyingMaterial& master_key, bool accept_no_keys)
{
    {
        LOCK(cs_KeyStore);
        assert(mapKeys.empty());
        bool keyPass = mapCryptedKeys.empty();
        bool keyFail = false;
        CryptedKeyMap::const_iterator mi = mapCryptedKeys.begin();
        WalletBatch batch(m_storage.GetDatabase());
        for (; mi != mapCryptedKeys.end(); ++mi)
        {
            const CPubKey &vchPubKey = (*mi).second.first;
            const std::vector<unsigned char> &vchCryptedSecret = (*mi).second.second;
            CKey key;
            if (!DecryptKey(master_key, vchCryptedSecret, vchPubKey, key))
            {
                keyFail = true;
                break;
            }
            keyPass = true;
            if (fDecryptionThoroughlyChecked)
                break;
            else {
                batch.WriteCryptedKey(vchPubKey, vchCryptedSecret, mapKeyMetadata[vchPubKey.GetID()]);
            }
        }
        if (keyPass && keyFail)
        {
            LogPrintf("The wallet is probably corrupted: Some keys decrypt but not all.\n");
            throw std::runtime_error("Error unlocking wallet: some keys decrypt but not all. Your wallet file may be corrupt.");
        }
        if (keyFail || (!keyPass && !accept_no_keys))
            return false;
        fDecryptionThoroughlyChecked = true;
    }
    return true;
}
bool LegacyScriptPubKeyMan::Encrypt(const CKeyingMaterial& master_key, WalletBatch* batch)
{
    LOCK(cs_KeyStore);
    encrypted_batch = batch;
    if (!mapCryptedKeys.empty()) {
        encrypted_batch = nullptr;
        return false;
    }
    KeyMap keys_to_encrypt;
    keys_to_encrypt.swap(mapKeys);
    for (const KeyMap::value_type& mKey : keys_to_encrypt)
    {
        const CKey &key = mKey.second;
        CPubKey vchPubKey = key.GetPubKey();
        CKeyingMaterial vchSecret(key.begin(), key.end());
        std::vector<unsigned char> vchCryptedSecret;
        if (!EncryptSecret(master_key, vchSecret, vchPubKey.GetHash(), vchCryptedSecret)) {
            encrypted_batch = nullptr;
            return false;
        }
        if (!AddCryptedKey(vchPubKey, vchCryptedSecret)) {
            encrypted_batch = nullptr;
            return false;
        }
    }
    encrypted_batch = nullptr;
    return true;
}
util::Result<CTxDestination> LegacyScriptPubKeyMan::GetReservedDestination(const OutputType type, bool internal, int64_t& index, CKeyPool& keypool)
{
    if (LEGACY_OUTPUT_TYPES.count(type) == 0) {
        return util::Error{_("Error: Legacy wallets only support the \"legacy\", \"p2sh-segwit\", and \"bech32\" address types")};
    }
    assert(type != OutputType::BECH32M);
    LOCK(cs_KeyStore);
    if (!CanGetAddresses(internal)) {
        return util::Error{_("Error: Keypool ran out, please call keypoolrefill first")};
    }
    if (!ReserveKeyFromKeyPool(index, keypool, internal)) {
        return util::Error{_("Error: Keypool ran out, please call keypoolrefill first")};
    }
    return GetDestinationForKey(keypool.vchPubKey, type);
}
bool LegacyScriptPubKeyMan::TopUpInactiveHDChain(const CKeyID seed_id, int64_t index, bool internal)
{
    LOCK(cs_KeyStore);
    auto it = m_inactive_hd_chains.find(seed_id);
    if (it == m_inactive_hd_chains.end()) {
        return false;
    }
    CHDChain& chain = it->second;
    if (internal) {
        chain.m_next_internal_index = std::max(chain.m_next_internal_index, index + 1);
    } else {
        chain.m_next_external_index = std::max(chain.m_next_external_index, index + 1);
    }
    TopUpChain(chain, 0);
    return true;
}
std::vector<WalletDestination> LegacyScriptPubKeyMan::MarkUnusedAddresses(const CScript& script)
{
    LOCK(cs_KeyStore);
    std::vector<WalletDestination> result;
    for (const auto& keyid : GetAffectedKeys(script, *this)) {
        std::map<CKeyID, int64_t>::const_iterator mi = m_pool_key_to_index.find(keyid);
        if (mi != m_pool_key_to_index.end()) {
            WalletLogPrintf("%s: Detected a used keypool key, mark all keypool keys up to this key as used\n", __func__);
            for (const auto& keypool : MarkReserveKeysAsUsed(mi->second)) {
                for (const auto& type : LEGACY_OUTPUT_TYPES) {
                    const auto& dest = GetDestinationForKey(keypool.vchPubKey, type);
                    result.push_back({dest, keypool.fInternal});
                }
            }
            if (!TopUp()) {
                WalletLogPrintf("%s: Topping up keypool failed (locked wallet)\n", __func__);
            }
        }
        auto it = mapKeyMetadata.find(keyid);
        if (it != mapKeyMetadata.end()){
            CKeyMetadata meta = it->second;
            if (!meta.hd_seed_id.IsNull() && meta.hd_seed_id != m_hd_chain.seed_id) {
                std::vector<uint32_t> path;
                if (meta.has_key_origin) {
                    path = meta.key_origin.path;
                } else if (!ParseHDKeypath(meta.hdKeypath, path)) {
                    WalletLogPrintf("%s: Adding inactive seed keys failed, invalid hdKeypath: %s\n",
                                    __func__,
                                    meta.hdKeypath);
                }
                if (path.size() != 3) {
                    WalletLogPrintf("%s: Adding inactive seed keys failed, invalid path size: %d, has_key_origin: %s\n",
                                    __func__,
                                    path.size(),
                                    meta.has_key_origin);
                } else {
                    bool internal = (path[1] & ~BIP32_HARDENED_KEY_LIMIT) != 0;
                    int64_t index = path[2] & ~BIP32_HARDENED_KEY_LIMIT;
                    if (!TopUpInactiveHDChain(meta.hd_seed_id, index, internal)) {
                        WalletLogPrintf("%s: Adding inactive seed keys failed\n", __func__);
                    }
                }
            }
        }
    }
    return result;
}
void LegacyScriptPubKeyMan::UpgradeKeyMetadata()
{
    LOCK(cs_KeyStore);
    if (m_storage.IsLocked() || m_storage.IsWalletFlagSet(WALLET_FLAG_KEY_ORIGIN_METADATA)) {
        return;
    }
    std::unique_ptr<WalletBatch> batch = std::make_unique<WalletBatch>(m_storage.GetDatabase());
    for (auto& meta_pair : mapKeyMetadata) {
        CKeyMetadata& meta = meta_pair.second;
        if (!meta.hd_seed_id.IsNull() && !meta.has_key_origin && meta.hdKeypath != "s") {
            CKey key;
            GetKey(meta.hd_seed_id, key);
            CExtKey masterKey;
            masterKey.SetSeed(key);
            CKeyID master_id = masterKey.key.GetPubKey().GetID();
            std::copy(master_id.begin(), master_id.begin() + 4, meta.key_origin.fingerprint);
            if (!ParseHDKeypath(meta.hdKeypath, meta.key_origin.path)) {
                throw std::runtime_error("Invalid stored hdKeypath");
            }
            meta.has_key_origin = true;
            if (meta.nVersion < CKeyMetadata::VERSION_WITH_KEY_ORIGIN) {
                meta.nVersion = CKeyMetadata::VERSION_WITH_KEY_ORIGIN;
            }
            CPubKey pubkey;
            if (GetPubKey(meta_pair.first, pubkey)) {
                batch->WriteKeyMetadata(meta, pubkey, true);
            }
        }
    }
}
bool LegacyScriptPubKeyMan::SetupGeneration(bool force)
{
    if ((CanGenerateKeys() && !force) || m_storage.IsLocked()) {
        return false;
    }
    SetHDSeed(GenerateNewSeed());
    if (!NewKeyPool()) {
        return false;
    }
    return true;
}
bool LegacyScriptPubKeyMan::IsHDEnabled() const
{
    return !m_hd_chain.seed_id.IsNull();
}
bool LegacyScriptPubKeyMan::CanGetAddresses(bool internal) const
{
    LOCK(cs_KeyStore);
    bool keypool_has_keys;
    if (internal && m_storage.CanSupportFeature(FEATURE_HD_SPLIT)) {
        keypool_has_keys = setInternalKeyPool.size() > 0;
    } else {
        keypool_has_keys = KeypoolCountExternalKeys() > 0;
    }
    if (!keypool_has_keys) {
        return CanGenerateKeys();
    }
    return keypool_has_keys;
}
bool LegacyScriptPubKeyMan::Upgrade(int prev_version, int new_version, bilingual_str& error)
{
    LOCK(cs_KeyStore);
    if (m_storage.IsWalletFlagSet(WALLET_FLAG_DISABLE_PRIVATE_KEYS)) {
        return true;
    }
    bool hd_upgrade = false;
    bool split_upgrade = false;
    if (IsFeatureSupported(new_version, FEATURE_HD) && !IsHDEnabled()) {
        WalletLogPrintf("Upgrading wallet to HD\n");
        m_storage.SetMinVersion(FEATURE_HD);
        CPubKey masterPubKey = GenerateNewSeed();
        SetHDSeed(masterPubKey);
        hd_upgrade = true;
    }
    if (!IsFeatureSupported(prev_version, FEATURE_HD_SPLIT) && IsFeatureSupported(new_version, FEATURE_HD_SPLIT)) {
        WalletLogPrintf("Upgrading wallet to use HD chain split\n");
        m_storage.SetMinVersion(FEATURE_PRE_SPLIT_KEYPOOL);
        split_upgrade = FEATURE_HD_SPLIT > prev_version;
        if (m_hd_chain.nVersion < CHDChain::VERSION_HD_CHAIN_SPLIT) {
            m_hd_chain.nVersion = CHDChain::VERSION_HD_CHAIN_SPLIT;
            if (!WalletBatch(m_storage.GetDatabase()).WriteHDChain(m_hd_chain)) {
                throw std::runtime_error(std::string(__func__) + ": writing chain failed");
            }
        }
    }
    if (split_upgrade) {
        MarkPreSplitKeys();
    }
    if (hd_upgrade) {
        if (!NewKeyPool()) {
            error = _("Unable to generate keys");
            return false;
        }
    }
    return true;
}
bool LegacyScriptPubKeyMan::HavePrivateKeys() const
{
    LOCK(cs_KeyStore);
    return !mapKeys.empty() || !mapCryptedKeys.empty();
}
void LegacyScriptPubKeyMan::RewriteDB()
{
    LOCK(cs_KeyStore);
    setInternalKeyPool.clear();
    setExternalKeyPool.clear();
    m_pool_key_to_index.clear();
}
static int64_t GetOldestKeyTimeInPool(const std::set<int64_t>& setKeyPool, WalletBatch& batch) {
    if (setKeyPool.empty()) {
        return GetTime();
    }
    CKeyPool keypool;
    int64_t nIndex = *(setKeyPool.begin());
    if (!batch.ReadPool(nIndex, keypool)) {
        throw std::runtime_error(std::string(__func__) + ": read oldest key in keypool failed");
    }
    assert(keypool.vchPubKey.IsValid());
    return keypool.nTime;
}
std::optional<int64_t> LegacyScriptPubKeyMan::GetOldestKeyPoolTime() const
{
    LOCK(cs_KeyStore);
    WalletBatch batch(m_storage.GetDatabase());
    int64_t oldestKey = GetOldestKeyTimeInPool(setExternalKeyPool, batch);
    if (IsHDEnabled() && m_storage.CanSupportFeature(FEATURE_HD_SPLIT)) {
        oldestKey = std::max(GetOldestKeyTimeInPool(setInternalKeyPool, batch), oldestKey);
        if (!set_pre_split_keypool.empty()) {
            oldestKey = std::max(GetOldestKeyTimeInPool(set_pre_split_keypool, batch), oldestKey);
        }
    }
    return oldestKey;
}
size_t LegacyScriptPubKeyMan::KeypoolCountExternalKeys() const
{
    LOCK(cs_KeyStore);
    return setExternalKeyPool.size() + set_pre_split_keypool.size();
}
unsigned int LegacyScriptPubKeyMan::GetKeyPoolSize() const
{
    LOCK(cs_KeyStore);
    return setInternalKeyPool.size() + setExternalKeyPool.size() + set_pre_split_keypool.size();
}
int64_t LegacyScriptPubKeyMan::GetTimeFirstKey() const
{
    LOCK(cs_KeyStore);
    return nTimeFirstKey;
}
std::unique_ptr<SigningProvider> LegacyScriptPubKeyMan::GetSolvingProvider(const CScript& script) const
{
    return std::make_unique<LegacySigningProvider>(*this);
}
bool LegacyScriptPubKeyMan::CanProvide(const CScript& script, SignatureData& sigdata)
{
    IsMineResult ismine = IsMineInner(*this, script, IsMineSigVersion::TOP,   false);
    if (ismine == IsMineResult::SPENDABLE || ismine == IsMineResult::WATCH_ONLY) {
        return true;
    } else {
        ProduceSignature(*this, DUMMY_SIGNATURE_CREATOR, script, sigdata);
        if (!sigdata.signatures.empty()) {
            bool has_privkeys = false;
            for (const auto& key_sig_pair : sigdata.signatures) {
                has_privkeys |= HaveKey(key_sig_pair.first);
            }
            return has_privkeys;
        }
        return false;
    }
}
bool LegacyScriptPubKeyMan::SignTransaction(CMutableTransaction& tx, const std::map<COutPoint, Coin>& coins, int sighash, std::map<int, bilingual_str>& input_errors) const
{
    return ::SignTransaction(tx, this, coins, sighash, input_errors);
}
SigningResult LegacyScriptPubKeyMan::SignMessage(const std::string& message, const PKHash& pkhash, std::string& str_sig) const
{
    CKey key;
    if (!GetKey(ToKeyID(pkhash), key)) {
        return SigningResult::PRIVATE_KEY_NOT_AVAILABLE;
    }
    if (MessageSign(key, message, str_sig)) {
        return SigningResult::OK;
    }
    return SigningResult::SIGNING_FAILED;
}
TransactionError LegacyScriptPubKeyMan::FillPSBT(PartiallySignedTransaction& psbtx, const PrecomputedTransactionData& txdata, int sighash_type, bool sign, bool bip32derivs, int* n_signed, bool finalize) const
{
    if (n_signed) {
        *n_signed = 0;
    }
    for (unsigned int i = 0; i < psbtx.tx->vin.size(); ++i) {
        const CTxIn& txin = psbtx.tx->vin[i];
        PSBTInput& input = psbtx.inputs.at(i);
        if (PSBTInputSigned(input)) {
            continue;
        }
        if (sign && input.sighash_type != std::nullopt && *input.sighash_type != sighash_type) {
            return TransactionError::SIGHASH_MISMATCH;
        }
        if (input.non_witness_utxo) {
            if (txin.prevout.n >= input.non_witness_utxo->vout.size()) {
                return TransactionError::MISSING_INPUTS;
            }
        } else if (input.witness_utxo.IsNull()) {
            continue;
        }
        SignatureData sigdata;
        input.FillSignatureData(sigdata);
        SignPSBTInput(HidingSigningProvider(this, !sign, !bip32derivs), psbtx, i, &txdata, sighash_type, nullptr, finalize);
        bool signed_one = PSBTInputSigned(input);
        if (n_signed && (signed_one || !sign)) {
            (*n_signed)++;
        }
    }
    for (unsigned int i = 0; i < psbtx.tx->vout.size(); ++i) {
        UpdatePSBTOutput(HidingSigningProvider(this, true, !bip32derivs), psbtx, i);
    }
    return TransactionError::OK;
}
std::unique_ptr<CKeyMetadata> LegacyScriptPubKeyMan::GetMetadata(const CTxDestination& dest) const
{
    LOCK(cs_KeyStore);
    CKeyID key_id = GetKeyForDestination(*this, dest);
    if (!key_id.IsNull()) {
        auto it = mapKeyMetadata.find(key_id);
        if (it != mapKeyMetadata.end()) {
            return std::make_unique<CKeyMetadata>(it->second);
        }
    }
    CScript scriptPubKey = GetScriptForDestination(dest);
    auto it = m_script_metadata.find(CScriptID(scriptPubKey));
    if (it != m_script_metadata.end()) {
        return std::make_unique<CKeyMetadata>(it->second);
    }
    return nullptr;
}
uint256 LegacyScriptPubKeyMan::GetID() const
{
    return uint256::ONE;
}
void LegacyScriptPubKeyMan::UpdateTimeFirstKey(int64_t nCreateTime)
{
    AssertLockHeld(cs_KeyStore);
    if (nCreateTime <= 1) {
        nTimeFirstKey = 1;
    } else if (!nTimeFirstKey || nCreateTime < nTimeFirstKey) {
        nTimeFirstKey = nCreateTime;
    }
}
bool LegacyScriptPubKeyMan::LoadKey(const CKey& key, const CPubKey &pubkey)
{
    return AddKeyPubKeyInner(key, pubkey);
}
bool LegacyScriptPubKeyMan::AddKeyPubKey(const CKey& secret, const CPubKey &pubkey)
{
    LOCK(cs_KeyStore);
    WalletBatch batch(m_storage.GetDatabase());
    return LegacyScriptPubKeyMan::AddKeyPubKeyWithDB(batch, secret, pubkey);
}
bool LegacyScriptPubKeyMan::AddKeyPubKeyWithDB(WalletBatch& batch, const CKey& secret, const CPubKey& pubkey)
{
    AssertLockHeld(cs_KeyStore);
    assert(!m_storage.IsWalletFlagSet(WALLET_FLAG_DISABLE_PRIVATE_KEYS));
    bool needsDB = !encrypted_batch;
    if (needsDB) {
        encrypted_batch = &batch;
    }
    if (!AddKeyPubKeyInner(secret, pubkey)) {
        if (needsDB) encrypted_batch = nullptr;
        return false;
    }
    if (needsDB) encrypted_batch = nullptr;
    CScript script;
    script = GetScriptForDestination(PKHash(pubkey));
    if (HaveWatchOnly(script)) {
        RemoveWatchOnly(script);
    }
    script = GetScriptForRawPubKey(pubkey);
    if (HaveWatchOnly(script)) {
        RemoveWatchOnly(script);
    }
    if (!m_storage.HasEncryptionKeys()) {
        return batch.WriteKey(pubkey,
                                                 secret.GetPrivKey(),
                                                 mapKeyMetadata[pubkey.GetID()]);
    }
    m_storage.UnsetBlankWalletFlag(batch);
    return true;
}
bool LegacyScriptPubKeyMan::LoadCScript(const CScript& redeemScript)
{
    if (redeemScript.size() > MAX_SCRIPT_ELEMENT_SIZE)
    {
        std::string strAddr = EncodeDestination(ScriptHash(redeemScript));
        WalletLogPrintf("%s: Warning: This wallet contains a redeemScript of size %i which exceeds maximum size %i thus can never be redeemed. Do not use address %s.\n", __func__, redeemScript.size(), MAX_SCRIPT_ELEMENT_SIZE, strAddr);
        return true;
    }
    return FillableSigningProvider::AddCScript(redeemScript);
}
void LegacyScriptPubKeyMan::LoadKeyMetadata(const CKeyID& keyID, const CKeyMetadata& meta)
{
    LOCK(cs_KeyStore);
    UpdateTimeFirstKey(meta.nCreateTime);
    mapKeyMetadata[keyID] = meta;
}
void LegacyScriptPubKeyMan::LoadScriptMetadata(const CScriptID& script_id, const CKeyMetadata& meta)
{
    LOCK(cs_KeyStore);
    UpdateTimeFirstKey(meta.nCreateTime);
    m_script_metadata[script_id] = meta;
}
bool LegacyScriptPubKeyMan::AddKeyPubKeyInner(const CKey& key, const CPubKey &pubkey)
{
    LOCK(cs_KeyStore);
    if (!m_storage.HasEncryptionKeys()) {
        return FillableSigningProvider::AddKeyPubKey(key, pubkey);
    }
    if (m_storage.IsLocked()) {
        return false;
    }
    std::vector<unsigned char> vchCryptedSecret;
    CKeyingMaterial vchSecret(key.begin(), key.end());
    if (!EncryptSecret(m_storage.GetEncryptionKey(), vchSecret, pubkey.GetHash(), vchCryptedSecret)) {
        return false;
    }
    if (!AddCryptedKey(pubkey, vchCryptedSecret)) {
        return false;
    }
    return true;
}
bool LegacyScriptPubKeyMan::LoadCryptedKey(const CPubKey &vchPubKey, const std::vector<unsigned char> &vchCryptedSecret, bool checksum_valid)
{
    if (!checksum_valid) {
        fDecryptionThoroughlyChecked = false;
    }
    return AddCryptedKeyInner(vchPubKey, vchCryptedSecret);
}
bool LegacyScriptPubKeyMan::AddCryptedKeyInner(const CPubKey &vchPubKey, const std::vector<unsigned char> &vchCryptedSecret)
{
    LOCK(cs_KeyStore);
    assert(mapKeys.empty());
    mapCryptedKeys[vchPubKey.GetID()] = make_pair(vchPubKey, vchCryptedSecret);
    ImplicitlyLearnRelatedKeyScripts(vchPubKey);
    return true;
}
bool LegacyScriptPubKeyMan::AddCryptedKey(const CPubKey &vchPubKey,
                            const std::vector<unsigned char> &vchCryptedSecret)
{
    if (!AddCryptedKeyInner(vchPubKey, vchCryptedSecret))
        return false;
    {
        LOCK(cs_KeyStore);
        if (encrypted_batch)
            return encrypted_batch->WriteCryptedKey(vchPubKey,
                                                        vchCryptedSecret,
                                                        mapKeyMetadata[vchPubKey.GetID()]);
        else
            return WalletBatch(m_storage.GetDatabase()).WriteCryptedKey(vchPubKey,
                                                            vchCryptedSecret,
                                                            mapKeyMetadata[vchPubKey.GetID()]);
    }
}
bool LegacyScriptPubKeyMan::HaveWatchOnly(const CScript &dest) const
{
    LOCK(cs_KeyStore);
    return setWatchOnly.count(dest) > 0;
}
bool LegacyScriptPubKeyMan::HaveWatchOnly() const
{
    LOCK(cs_KeyStore);
    return (!setWatchOnly.empty());
}
static bool ExtractPubKey(const CScript &dest, CPubKey& pubKeyOut)
{
    std::vector<std::vector<unsigned char>> solutions;
    return Solver(dest, solutions) == TxoutType::PUBKEY &&
        (pubKeyOut = CPubKey(solutions[0])).IsFullyValid();
}
bool LegacyScriptPubKeyMan::RemoveWatchOnly(const CScript &dest)
{
    {
        LOCK(cs_KeyStore);
        setWatchOnly.erase(dest);
        CPubKey pubKey;
        if (ExtractPubKey(dest, pubKey)) {
            mapWatchKeys.erase(pubKey.GetID());
        }
    }
    if (!HaveWatchOnly())
        NotifyWatchonlyChanged(false);
    if (!WalletBatch(m_storage.GetDatabase()).EraseWatchOnly(dest))
        return false;
    return true;
}
bool LegacyScriptPubKeyMan::LoadWatchOnly(const CScript &dest)
{
    return AddWatchOnlyInMem(dest);
}
bool LegacyScriptPubKeyMan::AddWatchOnlyInMem(const CScript &dest)
{
    LOCK(cs_KeyStore);
    setWatchOnly.insert(dest);
    CPubKey pubKey;
    if (ExtractPubKey(dest, pubKey)) {
        mapWatchKeys[pubKey.GetID()] = pubKey;
        ImplicitlyLearnRelatedKeyScripts(pubKey);
    }
    return true;
}
bool LegacyScriptPubKeyMan::AddWatchOnlyWithDB(WalletBatch &batch, const CScript& dest)
{
    if (!AddWatchOnlyInMem(dest))
        return false;
    const CKeyMetadata& meta = m_script_metadata[CScriptID(dest)];
    UpdateTimeFirstKey(meta.nCreateTime);
    NotifyWatchonlyChanged(true);
    if (batch.WriteWatchOnly(dest, meta)) {
        m_storage.UnsetBlankWalletFlag(batch);
        return true;
    }
    return false;
}
bool LegacyScriptPubKeyMan::AddWatchOnlyWithDB(WalletBatch &batch, const CScript& dest, int64_t create_time)
{
    m_script_metadata[CScriptID(dest)].nCreateTime = create_time;
    return AddWatchOnlyWithDB(batch, dest);
}
bool LegacyScriptPubKeyMan::AddWatchOnly(const CScript& dest)
{
    WalletBatch batch(m_storage.GetDatabase());
    return AddWatchOnlyWithDB(batch, dest);
}
bool LegacyScriptPubKeyMan::AddWatchOnly(const CScript& dest, int64_t nCreateTime)
{
    m_script_metadata[CScriptID(dest)].nCreateTime = nCreateTime;
    return AddWatchOnly(dest);
}
void LegacyScriptPubKeyMan::LoadHDChain(const CHDChain& chain)
{
    LOCK(cs_KeyStore);
    m_hd_chain = chain;
}
void LegacyScriptPubKeyMan::AddHDChain(const CHDChain& chain)
{
    LOCK(cs_KeyStore);
    if (!WalletBatch(m_storage.GetDatabase()).WriteHDChain(chain)) {
        throw std::runtime_error(std::string(__func__) + ": writing chain failed");
    }
    if (!m_hd_chain.seed_id.IsNull()) {
        AddInactiveHDChain(m_hd_chain);
    }
    m_hd_chain = chain;
}
void LegacyScriptPubKeyMan::AddInactiveHDChain(const CHDChain& chain)
{
    LOCK(cs_KeyStore);
    assert(!chain.seed_id.IsNull());
    m_inactive_hd_chains[chain.seed_id] = chain;
}
bool LegacyScriptPubKeyMan::HaveKey(const CKeyID &address) const
{
    LOCK(cs_KeyStore);
    if (!m_storage.HasEncryptionKeys()) {
        return FillableSigningProvider::HaveKey(address);
    }
    return mapCryptedKeys.count(address) > 0;
}
bool LegacyScriptPubKeyMan::GetKey(const CKeyID &address, CKey& keyOut) const
{
    LOCK(cs_KeyStore);
    if (!m_storage.HasEncryptionKeys()) {
        return FillableSigningProvider::GetKey(address, keyOut);
    }
    CryptedKeyMap::const_iterator mi = mapCryptedKeys.find(address);
    if (mi != mapCryptedKeys.end())
    {
        const CPubKey &vchPubKey = (*mi).second.first;
        const std::vector<unsigned char> &vchCryptedSecret = (*mi).second.second;
        return DecryptKey(m_storage.GetEncryptionKey(), vchCryptedSecret, vchPubKey, keyOut);
    }
    return false;
}
bool LegacyScriptPubKeyMan::GetKeyOrigin(const CKeyID& keyID, KeyOriginInfo& info) const
{
    CKeyMetadata meta;
    {
        LOCK(cs_KeyStore);
        auto it = mapKeyMetadata.find(keyID);
        if (it == mapKeyMetadata.end()) {
            return false;
        }
        meta = it->second;
    }
    if (meta.has_key_origin) {
        std::copy(meta.key_origin.fingerprint, meta.key_origin.fingerprint + 4, info.fingerprint);
        info.path = meta.key_origin.path;
    } else {
        std::copy(keyID.begin(), keyID.begin() + 4, info.fingerprint);
    }
    return true;
}
bool LegacyScriptPubKeyMan::GetWatchPubKey(const CKeyID &address, CPubKey &pubkey_out) const
{
    LOCK(cs_KeyStore);
    WatchKeyMap::const_iterator it = mapWatchKeys.find(address);
    if (it != mapWatchKeys.end()) {
        pubkey_out = it->second;
        return true;
    }
    return false;
}
bool LegacyScriptPubKeyMan::GetPubKey(const CKeyID &address, CPubKey& vchPubKeyOut) const
{
    LOCK(cs_KeyStore);
    if (!m_storage.HasEncryptionKeys()) {
        if (!FillableSigningProvider::GetPubKey(address, vchPubKeyOut)) {
            return GetWatchPubKey(address, vchPubKeyOut);
        }
        return true;
    }
    CryptedKeyMap::const_iterator mi = mapCryptedKeys.find(address);
    if (mi != mapCryptedKeys.end())
    {
        vchPubKeyOut = (*mi).second.first;
        return true;
    }
    return GetWatchPubKey(address, vchPubKeyOut);
}
CPubKey LegacyScriptPubKeyMan::GenerateNewKey(WalletBatch &batch, CHDChain& hd_chain, bool internal)
{
    assert(!m_storage.IsWalletFlagSet(WALLET_FLAG_DISABLE_PRIVATE_KEYS));
    assert(!m_storage.IsWalletFlagSet(WALLET_FLAG_BLANK_WALLET));
    AssertLockHeld(cs_KeyStore);
    bool fCompressed = m_storage.CanSupportFeature(FEATURE_COMPRPUBKEY);
    CKey secret;
    int64_t nCreationTime = GetTime();
    CKeyMetadata metadata(nCreationTime);
    if (IsHDEnabled()) {
        DeriveNewChildKey(batch, metadata, secret, hd_chain, (m_storage.CanSupportFeature(FEATURE_HD_SPLIT) ? internal : false));
    } else {
        secret.MakeNewKey(fCompressed);
    }
    if (fCompressed) {
        m_storage.SetMinVersion(FEATURE_COMPRPUBKEY);
    }
    CPubKey pubkey = secret.GetPubKey();
    assert(secret.VerifyPubKey(pubkey));
    mapKeyMetadata[pubkey.GetID()] = metadata;
    UpdateTimeFirstKey(nCreationTime);
    if (!AddKeyPubKeyWithDB(batch, secret, pubkey)) {
        throw std::runtime_error(std::string(__func__) + ": AddKey failed");
    }
    return pubkey;
}
static void DeriveExtKey(CExtKey& key_in, unsigned int index, CExtKey& key_out) {
    if (!key_in.Derive(key_out, index)) {
        throw std::runtime_error("Could not derive extended key");
    }
}
void LegacyScriptPubKeyMan::DeriveNewChildKey(WalletBatch &batch, CKeyMetadata& metadata, CKey& secret, CHDChain& hd_chain, bool internal)
{
    CKey seed;
    CExtKey masterKey;
    CExtKey accountKey;
    CExtKey chainChildKey;
    CExtKey childKey;
    if (!GetKey(hd_chain.seed_id, seed))
        throw std::runtime_error(std::string(__func__) + ": seed not found");
    masterKey.SetSeed(seed);
    DeriveExtKey(masterKey, BIP32_HARDENED_KEY_LIMIT, accountKey);
    assert(internal ? m_storage.CanSupportFeature(FEATURE_HD_SPLIT) : true);
    DeriveExtKey(accountKey, BIP32_HARDENED_KEY_LIMIT+(internal ? 1 : 0), chainChildKey);
    do {
        if (internal) {
            DeriveExtKey(chainChildKey, hd_chain.nInternalChainCounter | BIP32_HARDENED_KEY_LIMIT, childKey);
            metadata.hdKeypath = "m/0'/1'/" + ToString(hd_chain.nInternalChainCounter) + "'";
            metadata.key_origin.path.push_back(0 | BIP32_HARDENED_KEY_LIMIT);
            metadata.key_origin.path.push_back(1 | BIP32_HARDENED_KEY_LIMIT);
            metadata.key_origin.path.push_back(hd_chain.nInternalChainCounter | BIP32_HARDENED_KEY_LIMIT);
            hd_chain.nInternalChainCounter++;
        }
        else {
            DeriveExtKey(chainChildKey, hd_chain.nExternalChainCounter | BIP32_HARDENED_KEY_LIMIT, childKey);
            metadata.hdKeypath = "m/0'/0'/" + ToString(hd_chain.nExternalChainCounter) + "'";
            metadata.key_origin.path.push_back(0 | BIP32_HARDENED_KEY_LIMIT);
            metadata.key_origin.path.push_back(0 | BIP32_HARDENED_KEY_LIMIT);
            metadata.key_origin.path.push_back(hd_chain.nExternalChainCounter | BIP32_HARDENED_KEY_LIMIT);
            hd_chain.nExternalChainCounter++;
        }
    } while (HaveKey(childKey.key.GetPubKey().GetID()));
    secret = childKey.key;
    metadata.hd_seed_id = hd_chain.seed_id;
    CKeyID master_id = masterKey.key.GetPubKey().GetID();
    std::copy(master_id.begin(), master_id.begin() + 4, metadata.key_origin.fingerprint);
    metadata.has_key_origin = true;
    if (hd_chain.seed_id == m_hd_chain.seed_id && !batch.WriteHDChain(hd_chain))
        throw std::runtime_error(std::string(__func__) + ": writing HD chain model failed");
}
void LegacyScriptPubKeyMan::LoadKeyPool(int64_t nIndex, const CKeyPool &keypool)
{
    LOCK(cs_KeyStore);
    if (keypool.m_pre_split) {
        set_pre_split_keypool.insert(nIndex);
    } else if (keypool.fInternal) {
        setInternalKeyPool.insert(nIndex);
    } else {
        setExternalKeyPool.insert(nIndex);
    }
    m_max_keypool_index = std::max(m_max_keypool_index, nIndex);
    m_pool_key_to_index[keypool.vchPubKey.GetID()] = nIndex;
    CKeyID keyid = keypool.vchPubKey.GetID();
    if (mapKeyMetadata.count(keyid) == 0)
        mapKeyMetadata[keyid] = CKeyMetadata(keypool.nTime);
}
bool LegacyScriptPubKeyMan::CanGenerateKeys() const
{
    LOCK(cs_KeyStore);
    return IsHDEnabled() || !m_storage.CanSupportFeature(FEATURE_HD);
}
CPubKey LegacyScriptPubKeyMan::GenerateNewSeed()
{
    assert(!m_storage.IsWalletFlagSet(WALLET_FLAG_DISABLE_PRIVATE_KEYS));
    CKey key;
    key.MakeNewKey(true);
    return DeriveNewSeed(key);
}
CPubKey LegacyScriptPubKeyMan::DeriveNewSeed(const CKey& key)
{
    int64_t nCreationTime = GetTime();
    CKeyMetadata metadata(nCreationTime);
    CPubKey seed = key.GetPubKey();
    assert(key.VerifyPubKey(seed));
    metadata.hdKeypath     = "s";
    metadata.has_key_origin = false;
    metadata.hd_seed_id = seed.GetID();
    {
        LOCK(cs_KeyStore);
        mapKeyMetadata[seed.GetID()] = metadata;
        if (!AddKeyPubKey(key, seed))
            throw std::runtime_error(std::string(__func__) + ": AddKeyPubKey failed");
    }
    return seed;
}
void LegacyScriptPubKeyMan::SetHDSeed(const CPubKey& seed)
{
    LOCK(cs_KeyStore);
    CHDChain newHdChain;
    newHdChain.nVersion = m_storage.CanSupportFeature(FEATURE_HD_SPLIT) ? CHDChain::VERSION_HD_CHAIN_SPLIT : CHDChain::VERSION_HD_BASE;
    newHdChain.seed_id = seed.GetID();
    AddHDChain(newHdChain);
    NotifyCanGetAddressesChanged();
    WalletBatch batch(m_storage.GetDatabase());
    m_storage.UnsetBlankWalletFlag(batch);
}
bool LegacyScriptPubKeyMan::NewKeyPool()
{
    if (m_storage.IsWalletFlagSet(WALLET_FLAG_DISABLE_PRIVATE_KEYS)) {
        return false;
    }
    {
        LOCK(cs_KeyStore);
        WalletBatch batch(m_storage.GetDatabase());
        for (const int64_t nIndex : setInternalKeyPool) {
            batch.ErasePool(nIndex);
        }
        setInternalKeyPool.clear();
        for (const int64_t nIndex : setExternalKeyPool) {
            batch.ErasePool(nIndex);
        }
        setExternalKeyPool.clear();
        for (const int64_t nIndex : set_pre_split_keypool) {
            batch.ErasePool(nIndex);
        }
        set_pre_split_keypool.clear();
        m_pool_key_to_index.clear();
        if (!TopUp()) {
            return false;
        }
        WalletLogPrintf("LegacyScriptPubKeyMan::NewKeyPool rewrote keypool\n");
    }
    return true;
}
bool LegacyScriptPubKeyMan::TopUp(unsigned int kpSize)
{
    if (!CanGenerateKeys()) {
        return false;
    }
    if (!TopUpChain(m_hd_chain, kpSize)) {
        return false;
    }
    for (auto& [chain_id, chain] : m_inactive_hd_chains) {
        if (!TopUpChain(chain, kpSize)) {
            return false;
        }
    }
    NotifyCanGetAddressesChanged();
    return true;
}
bool LegacyScriptPubKeyMan::TopUpChain(CHDChain& chain, unsigned int kpSize)
{
    LOCK(cs_KeyStore);
    if (m_storage.IsLocked()) return false;
    unsigned int nTargetSize;
    if (kpSize > 0) {
        nTargetSize = kpSize;
    } else {
        nTargetSize = std::max(gArgs.GetIntArg("-keypool", DEFAULT_KEYPOOL_SIZE), int64_t{0});
    }
    int64_t target = std::max((int64_t) nTargetSize, int64_t{1});
    int64_t missingExternal;
    int64_t missingInternal;
    if (chain == m_hd_chain) {
        missingExternal = std::max(target - (int64_t)setExternalKeyPool.size(), int64_t{0});
        missingInternal = std::max(target - (int64_t)setInternalKeyPool.size(), int64_t{0});
    } else {
        missingExternal = std::max(target - (chain.nExternalChainCounter - chain.m_next_external_index), int64_t{0});
        missingInternal = std::max(target - (chain.nInternalChainCounter - chain.m_next_internal_index), int64_t{0});
    }
    if (!IsHDEnabled() || !m_storage.CanSupportFeature(FEATURE_HD_SPLIT)) {
        missingInternal = 0;
    }
    bool internal = false;
    WalletBatch batch(m_storage.GetDatabase());
    for (int64_t i = missingInternal + missingExternal; i--;) {
        if (i < missingInternal) {
            internal = true;
        }
        CPubKey pubkey(GenerateNewKey(batch, chain, internal));
        if (chain == m_hd_chain) {
            AddKeypoolPubkeyWithDB(pubkey, internal, batch);
        }
    }
    if (missingInternal + missingExternal > 0) {
        if (chain == m_hd_chain) {
            WalletLogPrintf("keypool added %d keys (%d internal), size=%u (%u internal)\n", missingInternal + missingExternal, missingInternal, setInternalKeyPool.size() + setExternalKeyPool.size() + set_pre_split_keypool.size(), setInternalKeyPool.size());
        } else {
            WalletLogPrintf("inactive seed with id %s added %d external keys, %d internal keys\n", HexStr(chain.seed_id), missingExternal, missingInternal);
        }
    }
    return true;
}
void LegacyScriptPubKeyMan::AddKeypoolPubkeyWithDB(const CPubKey& pubkey, const bool internal, WalletBatch& batch)
{
    LOCK(cs_KeyStore);
    assert(m_max_keypool_index < std::numeric_limits<int64_t>::max());
    int64_t index = ++m_max_keypool_index;
    if (!batch.WritePool(index, CKeyPool(pubkey, internal))) {
        throw std::runtime_error(std::string(__func__) + ": writing imported pubkey failed");
    }
    if (internal) {
        setInternalKeyPool.insert(index);
    } else {
        setExternalKeyPool.insert(index);
    }
    m_pool_key_to_index[pubkey.GetID()] = index;
}
void LegacyScriptPubKeyMan::KeepDestination(int64_t nIndex, const OutputType& type)
{
    assert(type != OutputType::BECH32M);
    WalletBatch batch(m_storage.GetDatabase());
    batch.ErasePool(nIndex);
    CPubKey pubkey;
    bool have_pk = GetPubKey(m_index_to_reserved_key.at(nIndex), pubkey);
    assert(have_pk);
    LearnRelatedScripts(pubkey, type);
    m_index_to_reserved_key.erase(nIndex);
    WalletLogPrintf("keypool keep %d\n", nIndex);
}
void LegacyScriptPubKeyMan::ReturnDestination(int64_t nIndex, bool fInternal, const CTxDestination&)
{
    {
        LOCK(cs_KeyStore);
        if (fInternal) {
            setInternalKeyPool.insert(nIndex);
        } else if (!set_pre_split_keypool.empty()) {
            set_pre_split_keypool.insert(nIndex);
        } else {
            setExternalKeyPool.insert(nIndex);
        }
        CKeyID& pubkey_id = m_index_to_reserved_key.at(nIndex);
        m_pool_key_to_index[pubkey_id] = nIndex;
        m_index_to_reserved_key.erase(nIndex);
        NotifyCanGetAddressesChanged();
    }
    WalletLogPrintf("keypool return %d\n", nIndex);
}
bool LegacyScriptPubKeyMan::GetKeyFromPool(CPubKey& result, const OutputType type, bool internal)
{
    assert(type != OutputType::BECH32M);
    if (!CanGetAddresses(internal)) {
        return false;
    }
    CKeyPool keypool;
    {
        LOCK(cs_KeyStore);
        int64_t nIndex;
        if (!ReserveKeyFromKeyPool(nIndex, keypool, internal) && !m_storage.IsWalletFlagSet(WALLET_FLAG_DISABLE_PRIVATE_KEYS)) {
            if (m_storage.IsLocked()) return false;
            WalletBatch batch(m_storage.GetDatabase());
            result = GenerateNewKey(batch, m_hd_chain, internal);
            return true;
        }
        KeepDestination(nIndex, type);
        result = keypool.vchPubKey;
    }
    return true;
}
bool LegacyScriptPubKeyMan::ReserveKeyFromKeyPool(int64_t& nIndex, CKeyPool& keypool, bool fRequestedInternal)
{
    nIndex = -1;
    keypool.vchPubKey = CPubKey();
    {
        LOCK(cs_KeyStore);
        bool fReturningInternal = fRequestedInternal;
        fReturningInternal &= (IsHDEnabled() && m_storage.CanSupportFeature(FEATURE_HD_SPLIT)) || m_storage.IsWalletFlagSet(WALLET_FLAG_DISABLE_PRIVATE_KEYS);
        bool use_split_keypool = set_pre_split_keypool.empty();
        std::set<int64_t>& setKeyPool = use_split_keypool ? (fReturningInternal ? setInternalKeyPool : setExternalKeyPool) : set_pre_split_keypool;
        if (setKeyPool.empty()) {
            return false;
        }
        WalletBatch batch(m_storage.GetDatabase());
        auto it = setKeyPool.begin();
        nIndex = *it;
        setKeyPool.erase(it);
        if (!batch.ReadPool(nIndex, keypool)) {
            throw std::runtime_error(std::string(__func__) + ": read failed");
        }
        CPubKey pk;
        if (!GetPubKey(keypool.vchPubKey.GetID(), pk)) {
            throw std::runtime_error(std::string(__func__) + ": unknown key in key pool");
        }
        if (use_split_keypool && keypool.fInternal != fReturningInternal) {
            throw std::runtime_error(std::string(__func__) + ": keypool entry misclassified");
        }
        if (!keypool.vchPubKey.IsValid()) {
            throw std::runtime_error(std::string(__func__) + ": keypool entry invalid");
        }
        assert(m_index_to_reserved_key.count(nIndex) == 0);
        m_index_to_reserved_key[nIndex] = keypool.vchPubKey.GetID();
        m_pool_key_to_index.erase(keypool.vchPubKey.GetID());
        WalletLogPrintf("keypool reserve %d\n", nIndex);
    }
    NotifyCanGetAddressesChanged();
    return true;
}
void LegacyScriptPubKeyMan::LearnRelatedScripts(const CPubKey& key, OutputType type)
{
    assert(type != OutputType::BECH32M);
    if (key.IsCompressed() && (type == OutputType::P2SH_SEGWIT || type == OutputType::BECH32)) {
        CTxDestination witdest = WitnessV0KeyHash(key.GetID());
        CScript witprog = GetScriptForDestination(witdest);
        const auto desc = InferDescriptor(witprog, *this);
        assert(desc && desc->IsSolvable());
        AddCScript(witprog);
    }
}
void LegacyScriptPubKeyMan::LearnAllRelatedScripts(const CPubKey& key)
{
    LearnRelatedScripts(key, OutputType::P2SH_SEGWIT);
}
std::vector<CKeyPool> LegacyScriptPubKeyMan::MarkReserveKeysAsUsed(int64_t keypool_id)
{
    AssertLockHeld(cs_KeyStore);
    bool internal = setInternalKeyPool.count(keypool_id);
    if (!internal) assert(setExternalKeyPool.count(keypool_id) || set_pre_split_keypool.count(keypool_id));
    std::set<int64_t> *setKeyPool = internal ? &setInternalKeyPool : (set_pre_split_keypool.empty() ? &setExternalKeyPool : &set_pre_split_keypool);
    auto it = setKeyPool->begin();
    std::vector<CKeyPool> result;
    WalletBatch batch(m_storage.GetDatabase());
    while (it != std::end(*setKeyPool)) {
        const int64_t& index = *(it);
        if (index > keypool_id) break;
        CKeyPool keypool;
        if (batch.ReadPool(index, keypool)) {
            m_pool_key_to_index.erase(keypool.vchPubKey.GetID());
        }
        LearnAllRelatedScripts(keypool.vchPubKey);
        batch.ErasePool(index);
        WalletLogPrintf("keypool index %d removed\n", index);
        it = setKeyPool->erase(it);
        result.push_back(std::move(keypool));
    }
    return result;
}
std::vector<CKeyID> GetAffectedKeys(const CScript& spk, const SigningProvider& provider)
{
    std::vector<CScript> dummy;
    FlatSigningProvider out;
    InferDescriptor(spk, provider)->Expand(0, DUMMY_SIGNING_PROVIDER, dummy, out);
    std::vector<CKeyID> ret;
    for (const auto& entry : out.pubkeys) {
        ret.push_back(entry.first);
    }
    return ret;
}
void LegacyScriptPubKeyMan::MarkPreSplitKeys()
{
    WalletBatch batch(m_storage.GetDatabase());
    for (auto it = setExternalKeyPool.begin(); it != setExternalKeyPool.end();) {
        int64_t index = *it;
        CKeyPool keypool;
        if (!batch.ReadPool(index, keypool)) {
            throw std::runtime_error(std::string(__func__) + ": read keypool entry failed");
        }
        keypool.m_pre_split = true;
        if (!batch.WritePool(index, keypool)) {
            throw std::runtime_error(std::string(__func__) + ": writing modified keypool entry failed");
        }
        set_pre_split_keypool.insert(index);
        it = setExternalKeyPool.erase(it);
    }
}
bool LegacyScriptPubKeyMan::AddCScript(const CScript& redeemScript)
{
    WalletBatch batch(m_storage.GetDatabase());
    return AddCScriptWithDB(batch, redeemScript);
}
bool LegacyScriptPubKeyMan::AddCScriptWithDB(WalletBatch& batch, const CScript& redeemScript)
{
    if (!FillableSigningProvider::AddCScript(redeemScript))
        return false;
    if (batch.WriteCScript(Hash160(redeemScript), redeemScript)) {
        m_storage.UnsetBlankWalletFlag(batch);
        return true;
    }
    return false;
}
bool LegacyScriptPubKeyMan::AddKeyOriginWithDB(WalletBatch& batch, const CPubKey& pubkey, const KeyOriginInfo& info)
{
    LOCK(cs_KeyStore);
    std::copy(info.fingerprint, info.fingerprint + 4, mapKeyMetadata[pubkey.GetID()].key_origin.fingerprint);
    mapKeyMetadata[pubkey.GetID()].key_origin.path = info.path;
    mapKeyMetadata[pubkey.GetID()].has_key_origin = true;
    mapKeyMetadata[pubkey.GetID()].hdKeypath = WriteHDKeypath(info.path);
    return batch.WriteKeyMetadata(mapKeyMetadata[pubkey.GetID()], pubkey, true);
}
bool LegacyScriptPubKeyMan::ImportScripts(const std::set<CScript> scripts, int64_t timestamp)
{
    WalletBatch batch(m_storage.GetDatabase());
    for (const auto& entry : scripts) {
        CScriptID id(entry);
        if (HaveCScript(id)) {
            WalletLogPrintf("Already have script %s, skipping\n", HexStr(entry));
            continue;
        }
        if (!AddCScriptWithDB(batch, entry)) {
            return false;
        }
        if (timestamp > 0) {
            m_script_metadata[CScriptID(entry)].nCreateTime = timestamp;
        }
    }
    if (timestamp > 0) {
        UpdateTimeFirstKey(timestamp);
    }
    return true;
}
bool LegacyScriptPubKeyMan::ImportPrivKeys(const std::map<CKeyID, CKey>& privkey_map, const int64_t timestamp)
{
    WalletBatch batch(m_storage.GetDatabase());
    for (const auto& entry : privkey_map) {
        const CKey& key = entry.second;
        CPubKey pubkey = key.GetPubKey();
        const CKeyID& id = entry.first;
        assert(key.VerifyPubKey(pubkey));
        if (HaveKey(id)) {
            WalletLogPrintf("Already have key with pubkey %s, skipping\n", HexStr(pubkey));
            continue;
        }
        mapKeyMetadata[id].nCreateTime = timestamp;
        if (!AddKeyPubKeyWithDB(batch, key, pubkey)) {
            return false;
        }
        UpdateTimeFirstKey(timestamp);
    }
    return true;
}
bool LegacyScriptPubKeyMan::ImportPubKeys(const std::vector<CKeyID>& ordered_pubkeys, const std::map<CKeyID, CPubKey>& pubkey_map, const std::map<CKeyID, std::pair<CPubKey, KeyOriginInfo>>& key_origins, const bool add_keypool, const bool internal, const int64_t timestamp)
{
    WalletBatch batch(m_storage.GetDatabase());
    for (const auto& entry : key_origins) {
        AddKeyOriginWithDB(batch, entry.second.first, entry.second.second);
    }
    for (const CKeyID& id : ordered_pubkeys) {
        auto entry = pubkey_map.find(id);
        if (entry == pubkey_map.end()) {
            continue;
        }
        const CPubKey& pubkey = entry->second;
        CPubKey temp;
        if (GetPubKey(id, temp)) {
            WalletLogPrintf("Already have pubkey %s, skipping\n", HexStr(temp));
            continue;
        }
        if (!AddWatchOnlyWithDB(batch, GetScriptForRawPubKey(pubkey), timestamp)) {
            return false;
        }
        mapKeyMetadata[id].nCreateTime = timestamp;
        if (add_keypool) {
            AddKeypoolPubkeyWithDB(pubkey, internal, batch);
            NotifyCanGetAddressesChanged();
        }
    }
    return true;
}
bool LegacyScriptPubKeyMan::ImportScriptPubKeys(const std::set<CScript>& script_pub_keys, const bool have_solving_data, const int64_t timestamp)
{
    WalletBatch batch(m_storage.GetDatabase());
    for (const CScript& script : script_pub_keys) {
        if (!have_solving_data || !IsMine(script)) {
            if (!AddWatchOnlyWithDB(batch, script, timestamp)) {
                return false;
            }
        }
    }
    return true;
}
std::set<CKeyID> LegacyScriptPubKeyMan::GetKeys() const
{
    LOCK(cs_KeyStore);
    if (!m_storage.HasEncryptionKeys()) {
        return FillableSigningProvider::GetKeys();
    }
    std::set<CKeyID> set_address;
    for (const auto& mi : mapCryptedKeys) {
        set_address.insert(mi.first);
    }
    return set_address;
}
const std::unordered_set<CScript, SaltedSipHasher> LegacyScriptPubKeyMan::GetScriptPubKeys() const
{
    LOCK(cs_KeyStore);
    std::unordered_set<CScript, SaltedSipHasher> spks;
    for (const auto& key_pair : mapKeys) {
        const CPubKey& pub = key_pair.second.GetPubKey();
        spks.insert(GetScriptForRawPubKey(pub));
        spks.insert(GetScriptForDestination(PKHash(pub)));
    }
    for (const auto& key_pair : mapCryptedKeys) {
        const CPubKey& pub = key_pair.second.first;
        spks.insert(GetScriptForRawPubKey(pub));
        spks.insert(GetScriptForDestination(PKHash(pub)));
    }
    for (const auto& script_pair : mapScripts) {
        const CScript& script = script_pair.second;
        if (IsMine(script) == ISMINE_SPENDABLE) {
            if (!script.IsPayToScriptHash()) {
                spks.insert(GetScriptForDestination(ScriptHash(script)));
            }
            int wit_ver = -1;
            std::vector<unsigned char> witprog;
            if (script.IsWitnessProgram(wit_ver, witprog) && wit_ver == 0) {
                spks.insert(script);
            }
        } else {
            std::vector<std::vector<unsigned char>> sols;
            TxoutType type = Solver(script, sols);
            if (type == TxoutType::MULTISIG) {
                CScript ms_spk = GetScriptForDestination(ScriptHash(script));
                if (IsMine(ms_spk) != ISMINE_NO) {
                    spks.insert(ms_spk);
                }
            }
        }
    }
    spks.insert(setWatchOnly.begin(), setWatchOnly.end());
    return spks;
}
std::optional<MigrationData> LegacyScriptPubKeyMan::MigrateToDescriptor()
{
    LOCK(cs_KeyStore);
    if (m_storage.IsLocked()) {
        return std::nullopt;
    }
    MigrationData out;
    std::unordered_set<CScript, SaltedSipHasher> spks{GetScriptPubKeys()};
    std::set<CKeyID> keyids;
    for (const auto& key_pair : mapKeys) {
        keyids.insert(key_pair.first);
    }
    for (const auto& key_pair : mapCryptedKeys) {
        keyids.insert(key_pair.first);
    }
    for (auto keyid_it = keyids.begin(); keyid_it != keyids.end();) {
        const CKeyID& keyid = *keyid_it;
        const auto& it = mapKeyMetadata.find(keyid);
        if (it != mapKeyMetadata.end()) {
            const CKeyMetadata& meta = it->second;
            if (meta.hdKeypath == "s" || meta.hdKeypath == "m") {
                keyid_it++;
                continue;
            }
            if (m_hd_chain.seed_id == meta.hd_seed_id || m_inactive_hd_chains.count(meta.hd_seed_id) > 0) {
                keyid_it = keyids.erase(keyid_it);
                continue;
            }
        }
        keyid_it++;
    }
    for (const CKeyID& keyid : keyids) {
        CKey key;
        if (!GetKey(keyid, key)) {
            assert(false);
        }
        uint64_t creation_time = 0;
        const auto& it = mapKeyMetadata.find(keyid);
        if (it != mapKeyMetadata.end()) {
            creation_time = it->second.nCreateTime;
        }
        KeyOriginInfo info;
        bool has_info = GetKeyOrigin(keyid, info);
        std::string origin_str = has_info ? "[" + HexStr(info.fingerprint) + FormatHDKeypath(info.path) + "]" : "";
        std::string desc_str = "combo(" + origin_str + HexStr(key.GetPubKey()) + ")";
        FlatSigningProvider keys;
        std::string error;
        std::unique_ptr<Descriptor> desc = Parse(desc_str, keys, error, false);
        WalletDescriptor w_desc(std::move(desc), creation_time, 0, 0, 0);
        auto desc_spk_man = std::unique_ptr<DescriptorScriptPubKeyMan>(new DescriptorScriptPubKeyMan(m_storage, w_desc));
        desc_spk_man->AddDescriptorKey(key, key.GetPubKey());
        desc_spk_man->TopUp();
        auto desc_spks = desc_spk_man->GetScriptPubKeys();
        for (const CScript& spk : desc_spks) {
            size_t erased = spks.erase(spk);
            assert(erased == 1);
            assert(IsMine(spk) == ISMINE_SPENDABLE);
        }
        out.desc_spkms.push_back(std::move(desc_spk_man));
    }
    std::vector<CHDChain> chains;
    chains.push_back(m_hd_chain);
    for (const auto& chain_pair : m_inactive_hd_chains) {
        chains.push_back(chain_pair.second);
    }
    for (const CHDChain& chain : chains) {
        for (int i = 0; i < 2; ++i) {
            if (chain.seed_id.IsNull() || (i == 1 && !m_storage.CanSupportFeature(FEATURE_HD_SPLIT))) {
                continue;
            }
            CKey seed_key;
            if (!GetKey(chain.seed_id, seed_key)) {
                assert(false);
            }
            CExtKey master_key;
            master_key.SetSeed(seed_key);
            std::string xpub = EncodeExtPubKey(master_key.Neuter());
            std::string desc_str = "combo(" + xpub + "/0'/" + ToString(i) + "'/*')";
            FlatSigningProvider keys;
            std::string error;
            std::unique_ptr<Descriptor> desc = Parse(desc_str, keys, error, false);
            uint32_t chain_counter = std::max((i == 1 ? chain.nInternalChainCounter : chain.nExternalChainCounter), (uint32_t)0);
            WalletDescriptor w_desc(std::move(desc), 0, 0, chain_counter, 0);
            auto desc_spk_man = std::unique_ptr<DescriptorScriptPubKeyMan>(new DescriptorScriptPubKeyMan(m_storage, w_desc));
            desc_spk_man->AddDescriptorKey(master_key.key, master_key.key.GetPubKey());
            desc_spk_man->TopUp();
            auto desc_spks = desc_spk_man->GetScriptPubKeys();
            for (const CScript& spk : desc_spks) {
                size_t erased = spks.erase(spk);
                assert(erased == 1);
                assert(IsMine(spk) == ISMINE_SPENDABLE);
            }
            out.desc_spkms.push_back(std::move(desc_spk_man));
        }
    }
    if (!m_hd_chain.seed_id.IsNull()) {
        CKey seed_key;
        if (!GetKey(m_hd_chain.seed_id, seed_key)) {
            assert(false);
        }
        out.master_key.SetSeed(seed_key);
    }
    for (auto it = spks.begin(); it != spks.end();) {
        const CScript& spk = *it;
        uint64_t creation_time = 0;
        const auto& mit = m_script_metadata.find(CScriptID(spk));
        if (mit != m_script_metadata.end()) {
            creation_time = mit->second.nCreateTime;
        }
        std::unique_ptr<Descriptor> desc = InferDescriptor(spk, *GetSolvingProvider(spk));
        std::vector<CScript> scripts;
        FlatSigningProvider keys;
        if (!desc->Expand(0, DUMMY_SIGNING_PROVIDER, scripts, keys)) {
            assert(false);
        }
        std::set<CKeyID> privkeyids;
        for (const auto& key_orig_pair : keys.origins) {
            privkeyids.insert(key_orig_pair.first);
        }
        std::vector<CScript> desc_spks;
        std::string desc_str;
        bool watchonly = !desc->ToPrivateString(*this, desc_str);
        if (watchonly && !m_storage.IsWalletFlagSet(WALLET_FLAG_DISABLE_PRIVATE_KEYS)) {
            out.watch_descs.push_back({desc->ToString(), creation_time});
            FlatSigningProvider provider;
            desc->Expand(0, provider, desc_spks, provider);
        } else {
            WalletDescriptor w_desc(std::move(desc), creation_time, 0, 0, 0);
            auto desc_spk_man = std::unique_ptr<DescriptorScriptPubKeyMan>(new DescriptorScriptPubKeyMan(m_storage, w_desc));
            for (const auto& keyid : privkeyids) {
                CKey key;
                if (!GetKey(keyid, key)) {
                    continue;
                }
                desc_spk_man->AddDescriptorKey(key, key.GetPubKey());
            }
            desc_spk_man->TopUp();
            auto desc_spks_set = desc_spk_man->GetScriptPubKeys();
            desc_spks.insert(desc_spks.end(), desc_spks_set.begin(), desc_spks_set.end());
            out.desc_spkms.push_back(std::move(desc_spk_man));
        }
        for (const CScript& desc_spk : desc_spks) {
            auto del_it = spks.find(desc_spk);
            assert(del_it != spks.end());
            assert(IsMine(desc_spk) != ISMINE_NO);
            it = spks.erase(del_it);
        }
    }
    for (const auto& script_pair : mapScripts) {
        const CScript script = script_pair.second;
        uint64_t creation_time = 0;
        const auto& it = m_script_metadata.find(CScriptID(script));
        if (it != m_script_metadata.end()) {
            creation_time = it->second.nCreateTime;
        }
        std::vector<std::vector<unsigned char>> sols;
        TxoutType type = Solver(script, sols);
        if (type == TxoutType::MULTISIG) {
            CScript sh_spk = GetScriptForDestination(ScriptHash(script));
            CTxDestination witdest = WitnessV0ScriptHash(script);
            CScript witprog = GetScriptForDestination(witdest);
            CScript sh_wsh_spk = GetScriptForDestination(ScriptHash(witprog));
            std::vector<std::vector<unsigned char>> keys(sols.begin() + 1, sols.begin() + sols.size() - 1);
            if (HaveWatchOnly(sh_spk) || HaveWatchOnly(script) || HaveKeys(keys, *this) || (HaveCScript(CScriptID(witprog)) && HaveWatchOnly(witprog))) {
                assert(IsMine(sh_spk) != ISMINE_NO || IsMine(witprog) != ISMINE_NO || IsMine(sh_wsh_spk) != ISMINE_NO);
                continue;
            }
            assert(IsMine(sh_spk) == ISMINE_NO && IsMine(witprog) == ISMINE_NO && IsMine(sh_wsh_spk) == ISMINE_NO);
            std::unique_ptr<Descriptor> sh_desc = InferDescriptor(sh_spk, *GetSolvingProvider(sh_spk));
            out.solvable_descs.push_back({sh_desc->ToString(), creation_time});
            const auto desc = InferDescriptor(witprog, *this);
            if (desc->IsSolvable()) {
                std::unique_ptr<Descriptor> wsh_desc = InferDescriptor(witprog, *GetSolvingProvider(witprog));
                out.solvable_descs.push_back({wsh_desc->ToString(), creation_time});
                std::unique_ptr<Descriptor> sh_wsh_desc = InferDescriptor(sh_wsh_spk, *GetSolvingProvider(sh_wsh_spk));
                out.solvable_descs.push_back({sh_wsh_desc->ToString(), creation_time});
            }
        }
    }
    assert(spks.size() == 0);
    return out;
}
bool LegacyScriptPubKeyMan::DeleteRecords()
{
    LOCK(cs_KeyStore);
    WalletBatch batch(m_storage.GetDatabase());
    return batch.EraseRecords(DBKeys::LEGACY_TYPES);
}
util::Result<CTxDestination> DescriptorScriptPubKeyMan::GetNewDestination(const OutputType type)
{
    if (!CanGetAddresses()) {
        return util::Error{_("No addresses available")};
    }
    {
        LOCK(cs_desc_man);
        assert(m_wallet_descriptor.descriptor->IsSingleType());
        std::optional<OutputType> desc_addr_type = m_wallet_descriptor.descriptor->GetOutputType();
        assert(desc_addr_type);
        if (type != *desc_addr_type) {
            throw std::runtime_error(std::string(__func__) + ": Types are inconsistent");
        }
        TopUp();
        FlatSigningProvider out_keys;
        std::vector<CScript> scripts_temp;
        if (m_wallet_descriptor.range_end <= m_max_cached_index && !TopUp(1)) {
            return util::Error{_("Error: Keypool ran out, please call keypoolrefill first")};
        }
        if (!m_wallet_descriptor.descriptor->ExpandFromCache(m_wallet_descriptor.next_index, m_wallet_descriptor.cache, scripts_temp, out_keys)) {
            return util::Error{_("Error: Keypool ran out, please call keypoolrefill first")};
        }
        CTxDestination dest;
        std::optional<OutputType> out_script_type = m_wallet_descriptor.descriptor->GetOutputType();
        if (out_script_type && out_script_type == type) {
            ExtractDestination(scripts_temp[0], dest);
        } else {
            throw std::runtime_error(std::string(__func__) + ": Types are inconsistent. Stored type does not match type of newly generated address");
        }
        m_wallet_descriptor.next_index++;
        WalletBatch(m_storage.GetDatabase()).WriteDescriptor(GetID(), m_wallet_descriptor);
        return dest;
    }
}
isminetype DescriptorScriptPubKeyMan::IsMine(const CScript& script) const
{
    LOCK(cs_desc_man);
    if (m_map_script_pub_keys.count(script) > 0) {
        return ISMINE_SPENDABLE;
    }
    return ISMINE_NO;
}
bool DescriptorScriptPubKeyMan::CheckDecryptionKey(const CKeyingMaterial& master_key, bool accept_no_keys)
{
    LOCK(cs_desc_man);
    if (!m_map_keys.empty()) {
        return false;
    }
    bool keyPass = m_map_crypted_keys.empty();
    bool keyFail = false;
    for (const auto& mi : m_map_crypted_keys) {
        const CPubKey &pubkey = mi.second.first;
        const std::vector<unsigned char> &crypted_secret = mi.second.second;
        CKey key;
        if (!DecryptKey(master_key, crypted_secret, pubkey, key)) {
            keyFail = true;
            break;
        }
        keyPass = true;
        if (m_decryption_thoroughly_checked)
            break;
    }
    if (keyPass && keyFail) {
        LogPrintf("The wallet is probably corrupted: Some keys decrypt but not all.\n");
        throw std::runtime_error("Error unlocking wallet: some keys decrypt but not all. Your wallet file may be corrupt.");
    }
    if (keyFail || (!keyPass && !accept_no_keys)) {
        return false;
    }
    m_decryption_thoroughly_checked = true;
    return true;
}
bool DescriptorScriptPubKeyMan::Encrypt(const CKeyingMaterial& master_key, WalletBatch* batch)
{
    LOCK(cs_desc_man);
    if (!m_map_crypted_keys.empty()) {
        return false;
    }
    for (const KeyMap::value_type& key_in : m_map_keys)
    {
        const CKey &key = key_in.second;
        CPubKey pubkey = key.GetPubKey();
        CKeyingMaterial secret(key.begin(), key.end());
        std::vector<unsigned char> crypted_secret;
        if (!EncryptSecret(master_key, secret, pubkey.GetHash(), crypted_secret)) {
            return false;
        }
        m_map_crypted_keys[pubkey.GetID()] = make_pair(pubkey, crypted_secret);
        batch->WriteCryptedDescriptorKey(GetID(), pubkey, crypted_secret);
    }
    m_map_keys.clear();
    return true;
}
util::Result<CTxDestination> DescriptorScriptPubKeyMan::GetReservedDestination(const OutputType type, bool internal, int64_t& index, CKeyPool& keypool)
{
    LOCK(cs_desc_man);
    auto op_dest = GetNewDestination(type);
    index = m_wallet_descriptor.next_index - 1;
    return op_dest;
}
void DescriptorScriptPubKeyMan::ReturnDestination(int64_t index, bool internal, const CTxDestination& addr)
{
    LOCK(cs_desc_man);
    if (m_wallet_descriptor.next_index - 1 == index) {
        m_wallet_descriptor.next_index--;
    }
    WalletBatch(m_storage.GetDatabase()).WriteDescriptor(GetID(), m_wallet_descriptor);
    NotifyCanGetAddressesChanged();
}
std::map<CKeyID, CKey> DescriptorScriptPubKeyMan::GetKeys() const
{
    AssertLockHeld(cs_desc_man);
    if (m_storage.HasEncryptionKeys() && !m_storage.IsLocked()) {
        KeyMap keys;
        for (const auto& key_pair : m_map_crypted_keys) {
            const CPubKey& pubkey = key_pair.second.first;
            const std::vector<unsigned char>& crypted_secret = key_pair.second.second;
            CKey key;
            DecryptKey(m_storage.GetEncryptionKey(), crypted_secret, pubkey, key);
            keys[pubkey.GetID()] = key;
        }
        return keys;
    }
    return m_map_keys;
}
bool DescriptorScriptPubKeyMan::TopUp(unsigned int size)
{
    LOCK(cs_desc_man);
    unsigned int target_size;
    if (size > 0) {
        target_size = size;
    } else {
        target_size = std::max(gArgs.GetIntArg("-keypool", DEFAULT_KEYPOOL_SIZE), (int64_t) 1);
    }
    int32_t new_range_end = std::max(m_wallet_descriptor.next_index + (int32_t)target_size, m_wallet_descriptor.range_end);
    if (!m_wallet_descriptor.descriptor->IsRange()) {
        new_range_end = 1;
        m_wallet_descriptor.range_end = 1;
        m_wallet_descriptor.range_start = 0;
    }
    FlatSigningProvider provider;
    provider.keys = GetKeys();
    WalletBatch batch(m_storage.GetDatabase());
    uint256 id = GetID();
    for (int32_t i = m_max_cached_index + 1; i < new_range_end; ++i) {
        FlatSigningProvider out_keys;
        std::vector<CScript> scripts_temp;
        DescriptorCache temp_cache;
        if (!m_wallet_descriptor.descriptor->ExpandFromCache(i, m_wallet_descriptor.cache, scripts_temp, out_keys)) {
            if (!m_wallet_descriptor.descriptor->Expand(i, provider, scripts_temp, out_keys, &temp_cache)) return false;
        }
        for (const CScript& script : scripts_temp) {
            m_map_script_pub_keys[script] = i;
        }
        for (const auto& pk_pair : out_keys.pubkeys) {
            const CPubKey& pubkey = pk_pair.second;
            if (m_map_pubkeys.count(pubkey) != 0) {
                continue;
            }
            m_map_pubkeys[pubkey] = i;
        }
        DescriptorCache new_items = m_wallet_descriptor.cache.MergeAndDiff(temp_cache);
        if (!batch.WriteDescriptorCacheItems(id, new_items)) {
            throw std::runtime_error(std::string(__func__) + ": writing cache items failed");
        }
        m_max_cached_index++;
    }
    m_wallet_descriptor.range_end = new_range_end;
    batch.WriteDescriptor(GetID(), m_wallet_descriptor);
    assert(m_wallet_descriptor.range_end - 1 == m_max_cached_index);
    NotifyCanGetAddressesChanged();
    return true;
}
std::vector<WalletDestination> DescriptorScriptPubKeyMan::MarkUnusedAddresses(const CScript& script)
{
    LOCK(cs_desc_man);
    std::vector<WalletDestination> result;
    if (IsMine(script)) {
        int32_t index = m_map_script_pub_keys[script];
        if (index >= m_wallet_descriptor.next_index) {
            WalletLogPrintf("%s: Detected a used keypool item at index %d, mark all keypool items up to this item as used\n", __func__, index);
            auto out_keys = std::make_unique<FlatSigningProvider>();
            std::vector<CScript> scripts_temp;
            while (index >= m_wallet_descriptor.next_index) {
                if (!m_wallet_descriptor.descriptor->ExpandFromCache(m_wallet_descriptor.next_index, m_wallet_descriptor.cache, scripts_temp, *out_keys)) {
                    throw std::runtime_error(std::string(__func__) + ": Unable to expand descriptor from cache");
                }
                CTxDestination dest;
                ExtractDestination(scripts_temp[0], dest);
                result.push_back({dest, std::nullopt});
                m_wallet_descriptor.next_index++;
            }
        }
        if (!TopUp()) {
            WalletLogPrintf("%s: Topping up keypool failed (locked wallet)\n", __func__);
        }
    }
    return result;
}
void DescriptorScriptPubKeyMan::AddDescriptorKey(const CKey& key, const CPubKey &pubkey)
{
    LOCK(cs_desc_man);
    WalletBatch batch(m_storage.GetDatabase());
    if (!AddDescriptorKeyWithDB(batch, key, pubkey)) {
        throw std::runtime_error(std::string(__func__) + ": writing descriptor private key failed");
    }
}
bool DescriptorScriptPubKeyMan::AddDescriptorKeyWithDB(WalletBatch& batch, const CKey& key, const CPubKey &pubkey)
{
    AssertLockHeld(cs_desc_man);
    assert(!m_storage.IsWalletFlagSet(WALLET_FLAG_DISABLE_PRIVATE_KEYS));
    if (m_map_keys.find(pubkey.GetID()) != m_map_keys.end() ||
        m_map_crypted_keys.find(pubkey.GetID()) != m_map_crypted_keys.end()) {
        return true;
    }
    if (m_storage.HasEncryptionKeys()) {
        if (m_storage.IsLocked()) {
            return false;
        }
        std::vector<unsigned char> crypted_secret;
        CKeyingMaterial secret(key.begin(), key.end());
        if (!EncryptSecret(m_storage.GetEncryptionKey(), secret, pubkey.GetHash(), crypted_secret)) {
            return false;
        }
        m_map_crypted_keys[pubkey.GetID()] = make_pair(pubkey, crypted_secret);
        return batch.WriteCryptedDescriptorKey(GetID(), pubkey, crypted_secret);
    } else {
        m_map_keys[pubkey.GetID()] = key;
        return batch.WriteDescriptorKey(GetID(), pubkey, key.GetPrivKey());
    }
}
bool DescriptorScriptPubKeyMan::SetupDescriptorGeneration(const CExtKey& master_key, OutputType addr_type, bool internal)
{
    LOCK(cs_desc_man);
    assert(m_storage.IsWalletFlagSet(WALLET_FLAG_DESCRIPTORS));
    if (m_wallet_descriptor.descriptor) {
        return false;
    }
    int64_t creation_time = GetTime();
    std::string xpub = EncodeExtPubKey(master_key.Neuter());
    std::string desc_prefix;
    std::string desc_suffix = "/*)";
    switch (addr_type) {
    case OutputType::LEGACY: {
        desc_prefix = "pkh(" + xpub + "/44'";
        break;
    }
    case OutputType::P2SH_SEGWIT: {
        desc_prefix = "sh(wpkh(" + xpub + "/49'";
        desc_suffix += ")";
        break;
    }
    case OutputType::BECH32: {
        desc_prefix = "wpkh(" + xpub + "/84'";
        break;
    }
    case OutputType::BECH32M: {
        desc_prefix = "tr(" + xpub  + "/86'";
        break;
    }
    case OutputType::UNKNOWN: {
        assert(false);
    }
    }
    assert(!desc_prefix.empty());
    if (Params().IsTestChain()) {
        desc_prefix += "/1'";
    } else {
        desc_prefix += "/0'";
    }
    std::string internal_path = internal ? "/1" : "/0";
    std::string desc_str = desc_prefix + "/0'" + internal_path + desc_suffix;
    FlatSigningProvider keys;
    std::string error;
    std::unique_ptr<Descriptor> desc = Parse(desc_str, keys, error, false);
    WalletDescriptor w_desc(std::move(desc), creation_time, 0, 0, 0);
    m_wallet_descriptor = w_desc;
    WalletBatch batch(m_storage.GetDatabase());
    if (!AddDescriptorKeyWithDB(batch, master_key.key, master_key.key.GetPubKey())) {
        throw std::runtime_error(std::string(__func__) + ": writing descriptor master private key failed");
    }
    if (!batch.WriteDescriptor(GetID(), m_wallet_descriptor)) {
        throw std::runtime_error(std::string(__func__) + ": writing descriptor failed");
    }
    TopUp();
    m_storage.UnsetBlankWalletFlag(batch);
    return true;
}
bool DescriptorScriptPubKeyMan::IsHDEnabled() const
{
    LOCK(cs_desc_man);
    return m_wallet_descriptor.descriptor->IsRange();
}
bool DescriptorScriptPubKeyMan::CanGetAddresses(bool internal) const
{
    LOCK(cs_desc_man);
    return m_wallet_descriptor.descriptor->IsSingleType() &&
           m_wallet_descriptor.descriptor->IsRange() &&
           (HavePrivateKeys() || m_wallet_descriptor.next_index < m_wallet_descriptor.range_end);
}
bool DescriptorScriptPubKeyMan::HavePrivateKeys() const
{
    LOCK(cs_desc_man);
    return m_map_keys.size() > 0 || m_map_crypted_keys.size() > 0;
}
std::optional<int64_t> DescriptorScriptPubKeyMan::GetOldestKeyPoolTime() const
{
    return std::nullopt;
}
unsigned int DescriptorScriptPubKeyMan::GetKeyPoolSize() const
{
    LOCK(cs_desc_man);
    return m_wallet_descriptor.range_end - m_wallet_descriptor.next_index;
}
int64_t DescriptorScriptPubKeyMan::GetTimeFirstKey() const
{
    LOCK(cs_desc_man);
    return m_wallet_descriptor.creation_time;
}
std::unique_ptr<FlatSigningProvider> DescriptorScriptPubKeyMan::GetSigningProvider(const CScript& script, bool include_private) const
{
    LOCK(cs_desc_man);
    auto it = m_map_script_pub_keys.find(script);
    if (it == m_map_script_pub_keys.end()) {
        return nullptr;
    }
    int32_t index = it->second;
    return GetSigningProvider(index, include_private);
}
std::unique_ptr<FlatSigningProvider> DescriptorScriptPubKeyMan::GetSigningProvider(const CPubKey& pubkey) const
{
    LOCK(cs_desc_man);
    auto it = m_map_pubkeys.find(pubkey);
    if (it == m_map_pubkeys.end()) {
        return nullptr;
    }
    int32_t index = it->second;
    return GetSigningProvider(index, true);
}
std::unique_ptr<FlatSigningProvider> DescriptorScriptPubKeyMan::GetSigningProvider(int32_t index, bool include_private) const
{
    AssertLockHeld(cs_desc_man);
    std::unique_ptr<FlatSigningProvider> out_keys = std::make_unique<FlatSigningProvider>();
    auto it = m_map_signing_providers.find(index);
    if (it != m_map_signing_providers.end()) {
        out_keys->Merge(FlatSigningProvider{it->second});
    } else {
        std::vector<CScript> scripts_temp;
        if (!m_wallet_descriptor.descriptor->ExpandFromCache(index, m_wallet_descriptor.cache, scripts_temp, *out_keys)) return nullptr;
        m_map_signing_providers[index] = *out_keys;
    }
    if (HavePrivateKeys() && include_private) {
        FlatSigningProvider master_provider;
        master_provider.keys = GetKeys();
        m_wallet_descriptor.descriptor->ExpandPrivate(index, master_provider, *out_keys);
    }
    return out_keys;
}
std::unique_ptr<SigningProvider> DescriptorScriptPubKeyMan::GetSolvingProvider(const CScript& script) const
{
    return GetSigningProvider(script, false);
}
bool DescriptorScriptPubKeyMan::CanProvide(const CScript& script, SignatureData& sigdata)
{
    return IsMine(script);
}
bool DescriptorScriptPubKeyMan::SignTransaction(CMutableTransaction& tx, const std::map<COutPoint, Coin>& coins, int sighash, std::map<int, bilingual_str>& input_errors) const
{
    std::unique_ptr<FlatSigningProvider> keys = std::make_unique<FlatSigningProvider>();
    for (const auto& coin_pair : coins) {
        std::unique_ptr<FlatSigningProvider> coin_keys = GetSigningProvider(coin_pair.second.out.scriptPubKey, true);
        if (!coin_keys) {
            continue;
        }
        keys->Merge(std::move(*coin_keys));
    }
    return ::SignTransaction(tx, keys.get(), coins, sighash, input_errors);
}
SigningResult DescriptorScriptPubKeyMan::SignMessage(const std::string& message, const PKHash& pkhash, std::string& str_sig) const
{
    std::unique_ptr<FlatSigningProvider> keys = GetSigningProvider(GetScriptForDestination(pkhash), true);
    if (!keys) {
        return SigningResult::PRIVATE_KEY_NOT_AVAILABLE;
    }
    CKey key;
    if (!keys->GetKey(ToKeyID(pkhash), key)) {
        return SigningResult::PRIVATE_KEY_NOT_AVAILABLE;
    }
    if (!MessageSign(key, message, str_sig)) {
        return SigningResult::SIGNING_FAILED;
    }
    return SigningResult::OK;
}
TransactionError DescriptorScriptPubKeyMan::FillPSBT(PartiallySignedTransaction& psbtx, const PrecomputedTransactionData& txdata, int sighash_type, bool sign, bool bip32derivs, int* n_signed, bool finalize) const
{
    if (n_signed) {
        *n_signed = 0;
    }
    for (unsigned int i = 0; i < psbtx.tx->vin.size(); ++i) {
        const CTxIn& txin = psbtx.tx->vin[i];
        PSBTInput& input = psbtx.inputs.at(i);
        if (PSBTInputSigned(input)) {
            continue;
        }
        if (sign && input.sighash_type != std::nullopt && *input.sighash_type != sighash_type) {
            return TransactionError::SIGHASH_MISMATCH;
        }
        CScript script;
        if (!input.witness_utxo.IsNull()) {
            script = input.witness_utxo.scriptPubKey;
        } else if (input.non_witness_utxo) {
            if (txin.prevout.n >= input.non_witness_utxo->vout.size()) {
                return TransactionError::MISSING_INPUTS;
            }
            script = input.non_witness_utxo->vout[txin.prevout.n].scriptPubKey;
        } else {
            continue;
        }
        SignatureData sigdata;
        input.FillSignatureData(sigdata);
        std::unique_ptr<FlatSigningProvider> keys = std::make_unique<FlatSigningProvider>();
        std::unique_ptr<FlatSigningProvider> script_keys = GetSigningProvider(script, sign);
        if (script_keys) {
            keys->Merge(std::move(*script_keys));
        } else {
            script_keys = std::make_unique<FlatSigningProvider>();
            for (const auto& pk_pair : input.hd_keypaths) {
                const CPubKey& pubkey = pk_pair.first;
                std::unique_ptr<FlatSigningProvider> pk_keys = GetSigningProvider(pubkey);
                if (pk_keys) {
                    keys->Merge(std::move(*pk_keys));
                }
            }
            for (const auto& pk_pair : input.m_tap_bip32_paths) {
                const XOnlyPubKey& pubkey = pk_pair.first;
                for (unsigned char prefix : {0x02, 0x03}) {
                    unsigned char b[33] = {prefix};
                    std::copy(pubkey.begin(), pubkey.end(), b + 1);
                    CPubKey fullpubkey;
                    fullpubkey.Set(b, b + 33);
                    std::unique_ptr<FlatSigningProvider> pk_keys = GetSigningProvider(fullpubkey);
                    if (pk_keys) {
                        keys->Merge(std::move(*pk_keys));
                    }
                }
            }
        }
        SignPSBTInput(HidingSigningProvider(keys.get(), !sign, !bip32derivs), psbtx, i, &txdata, sighash_type, nullptr, finalize);
        bool signed_one = PSBTInputSigned(input);
        if (n_signed && (signed_one || !sign)) {
            (*n_signed)++;
        }
    }
    for (unsigned int i = 0; i < psbtx.tx->vout.size(); ++i) {
        std::unique_ptr<SigningProvider> keys = GetSolvingProvider(psbtx.tx->vout.at(i).scriptPubKey);
        if (!keys) {
            continue;
        }
        UpdatePSBTOutput(HidingSigningProvider(keys.get(), true, !bip32derivs), psbtx, i);
    }
    return TransactionError::OK;
}
std::unique_ptr<CKeyMetadata> DescriptorScriptPubKeyMan::GetMetadata(const CTxDestination& dest) const
{
    std::unique_ptr<SigningProvider> provider = GetSigningProvider(GetScriptForDestination(dest));
    if (provider) {
        KeyOriginInfo orig;
        CKeyID key_id = GetKeyForDestination(*provider, dest);
        if (provider->GetKeyOrigin(key_id, orig)) {
            LOCK(cs_desc_man);
            std::unique_ptr<CKeyMetadata> meta = std::make_unique<CKeyMetadata>();
            meta->key_origin = orig;
            meta->has_key_origin = true;
            meta->nCreateTime = m_wallet_descriptor.creation_time;
            return meta;
        }
    }
    return nullptr;
}
uint256 DescriptorScriptPubKeyMan::GetID() const
{
    LOCK(cs_desc_man);
    std::string desc_str = m_wallet_descriptor.descriptor->ToString();
    uint256 id;
    CSHA256().Write((unsigned char*)desc_str.data(), desc_str.size()).Finalize(id.begin());
    return id;
}
void DescriptorScriptPubKeyMan::SetCache(const DescriptorCache& cache)
{
    LOCK(cs_desc_man);
    m_wallet_descriptor.cache = cache;
    for (int32_t i = m_wallet_descriptor.range_start; i < m_wallet_descriptor.range_end; ++i) {
        FlatSigningProvider out_keys;
        std::vector<CScript> scripts_temp;
        if (!m_wallet_descriptor.descriptor->ExpandFromCache(i, m_wallet_descriptor.cache, scripts_temp, out_keys)) {
            throw std::runtime_error("Error: Unable to expand wallet descriptor from cache");
        }
        for (const CScript& script : scripts_temp) {
            if (m_map_script_pub_keys.count(script) != 0) {
                throw std::runtime_error(strprintf("Error: Already loaded script at index %d as being at index %d", i, m_map_script_pub_keys[script]));
            }
            m_map_script_pub_keys[script] = i;
        }
        for (const auto& pk_pair : out_keys.pubkeys) {
            const CPubKey& pubkey = pk_pair.second;
            if (m_map_pubkeys.count(pubkey) != 0) {
                continue;
            }
            m_map_pubkeys[pubkey] = i;
        }
        m_max_cached_index++;
    }
}
bool DescriptorScriptPubKeyMan::AddKey(const CKeyID& key_id, const CKey& key)
{
    LOCK(cs_desc_man);
    m_map_keys[key_id] = key;
    return true;
}
bool DescriptorScriptPubKeyMan::AddCryptedKey(const CKeyID& key_id, const CPubKey& pubkey, const std::vector<unsigned char>& crypted_key)
{
    LOCK(cs_desc_man);
    if (!m_map_keys.empty()) {
        return false;
    }
    m_map_crypted_keys[key_id] = make_pair(pubkey, crypted_key);
    return true;
}
bool DescriptorScriptPubKeyMan::HasWalletDescriptor(const WalletDescriptor& desc) const
{
    LOCK(cs_desc_man);
    return m_wallet_descriptor.descriptor != nullptr && desc.descriptor != nullptr && m_wallet_descriptor.descriptor->ToString() == desc.descriptor->ToString();
}
void DescriptorScriptPubKeyMan::WriteDescriptor()
{
    LOCK(cs_desc_man);
    WalletBatch batch(m_storage.GetDatabase());
    if (!batch.WriteDescriptor(GetID(), m_wallet_descriptor)) {
        throw std::runtime_error(std::string(__func__) + ": writing descriptor failed");
    }
}
const WalletDescriptor DescriptorScriptPubKeyMan::GetWalletDescriptor() const
{
    return m_wallet_descriptor;
}
const std::unordered_set<CScript, SaltedSipHasher> DescriptorScriptPubKeyMan::GetScriptPubKeys() const
{
    LOCK(cs_desc_man);
    std::unordered_set<CScript, SaltedSipHasher> script_pub_keys;
    script_pub_keys.reserve(m_map_script_pub_keys.size());
    for (auto const& script_pub_key: m_map_script_pub_keys) {
        script_pub_keys.insert(script_pub_key.first);
    }
    return script_pub_keys;
}
bool DescriptorScriptPubKeyMan::GetDescriptorString(std::string& out, const bool priv) const
{
    LOCK(cs_desc_man);
    FlatSigningProvider provider;
    provider.keys = GetKeys();
    if (priv) {
        return m_wallet_descriptor.descriptor->ToPrivateString(provider, out);
    }
    return m_wallet_descriptor.descriptor->ToNormalizedString(provider, out, &m_wallet_descriptor.cache);
}
void DescriptorScriptPubKeyMan::UpgradeDescriptorCache()
{
    LOCK(cs_desc_man);
    if (m_storage.IsLocked() || m_storage.IsWalletFlagSet(WALLET_FLAG_LAST_HARDENED_XPUB_CACHED)) {
        return;
    }
    if (m_wallet_descriptor.cache.GetCachedLastHardenedExtPubKeys().size() > 0) {
        return;
    }
    FlatSigningProvider provider;
    provider.keys = GetKeys();
    FlatSigningProvider out_keys;
    std::vector<CScript> scripts_temp;
    DescriptorCache temp_cache;
    if (!m_wallet_descriptor.descriptor->Expand(0, provider, scripts_temp, out_keys, &temp_cache)){
        throw std::runtime_error("Unable to expand descriptor");
    }
    DescriptorCache diff = m_wallet_descriptor.cache.MergeAndDiff(temp_cache);
    if (!WalletBatch(m_storage.GetDatabase()).WriteDescriptorCacheItems(GetID(), diff)) {
        throw std::runtime_error(std::string(__func__) + ": writing cache items failed");
    }
}
void DescriptorScriptPubKeyMan::UpdateWalletDescriptor(WalletDescriptor& descriptor)
{
    LOCK(cs_desc_man);
    std::string error;
    if (!CanUpdateToWalletDescriptor(descriptor, error)) {
        throw std::runtime_error(std::string(__func__) + ": " + error);
    }
    m_map_pubkeys.clear();
    m_map_script_pub_keys.clear();
    m_max_cached_index = -1;
    m_wallet_descriptor = descriptor;
}
bool DescriptorScriptPubKeyMan::CanUpdateToWalletDescriptor(const WalletDescriptor& descriptor, std::string& error)
{
    LOCK(cs_desc_man);
    if (!HasWalletDescriptor(descriptor)) {
        error = "can only update matching descriptor";
        return false;
    }
    if (descriptor.range_start > m_wallet_descriptor.range_start ||
        descriptor.range_end < m_wallet_descriptor.range_end) {
        error = strprintf("new range must include current range = [%d,%d]",
                          m_wallet_descriptor.range_start,
                          m_wallet_descriptor.range_end - 1);
        return false;
    }
    return true;
}
}
