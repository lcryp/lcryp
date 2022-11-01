#ifndef LCRYP_WALLET_WALLETDB_H
#define LCRYP_WALLET_WALLETDB_H
#include <script/sign.h>
#include <wallet/db.h>
#include <wallet/walletutil.h>
#include <key.h>
#include <stdint.h>
#include <string>
#include <vector>
class CScript;
class uint160;
class uint256;
struct CBlockLocator;
namespace wallet {
class CKeyPool;
class CMasterKey;
class CWallet;
class CWalletTx;
struct WalletContext;
static const bool DEFAULT_FLUSHWALLET = true;
enum class DBErrors
{
    LOAD_OK,
    CORRUPT,
    NONCRITICAL_ERROR,
    TOO_NEW,
    EXTERNAL_SIGNER_SUPPORT_REQUIRED,
    LOAD_FAIL,
    NEED_REWRITE,
    NEED_RESCAN,
    UNKNOWN_DESCRIPTOR
};
namespace DBKeys {
extern const std::string ACENTRY;
extern const std::string ACTIVEEXTERNALSPK;
extern const std::string ACTIVEINTERNALSPK;
extern const std::string BESTBLOCK;
extern const std::string BESTBLOCK_NOMERKLE;
extern const std::string CRYPTED_KEY;
extern const std::string CSCRIPT;
extern const std::string DEFAULTKEY;
extern const std::string DESTDATA;
extern const std::string FLAGS;
extern const std::string HDCHAIN;
extern const std::string KEY;
extern const std::string KEYMETA;
extern const std::string LOCKED_UTXO;
extern const std::string MASTER_KEY;
extern const std::string MINVERSION;
extern const std::string NAME;
extern const std::string OLD_KEY;
extern const std::string ORDERPOSNEXT;
extern const std::string POOL;
extern const std::string PURPOSE;
extern const std::string SETTINGS;
extern const std::string TX;
extern const std::string VERSION;
extern const std::string WALLETDESCRIPTOR;
extern const std::string WALLETDESCRIPTORCKEY;
extern const std::string WALLETDESCRIPTORKEY;
extern const std::string WATCHMETA;
extern const std::string WATCHS;
extern const std::unordered_set<std::string> LEGACY_TYPES;
}
class CHDChain
{
public:
    uint32_t nExternalChainCounter;
    uint32_t nInternalChainCounter;
    CKeyID seed_id;
    int64_t m_next_external_index{0};
    int64_t m_next_internal_index{0};
    static const int VERSION_HD_BASE        = 1;
    static const int VERSION_HD_CHAIN_SPLIT = 2;
    static const int CURRENT_VERSION        = VERSION_HD_CHAIN_SPLIT;
    int nVersion;
    CHDChain() { SetNull(); }
    SERIALIZE_METHODS(CHDChain, obj)
    {
        READWRITE(obj.nVersion, obj.nExternalChainCounter, obj.seed_id);
        if (obj.nVersion >= VERSION_HD_CHAIN_SPLIT) {
            READWRITE(obj.nInternalChainCounter);
        }
    }
    void SetNull()
    {
        nVersion = CHDChain::CURRENT_VERSION;
        nExternalChainCounter = 0;
        nInternalChainCounter = 0;
        seed_id.SetNull();
    }
    bool operator==(const CHDChain& chain) const
    {
        return seed_id == chain.seed_id;
    }
};
class CKeyMetadata
{
public:
    static const int VERSION_BASIC=1;
    static const int VERSION_WITH_HDDATA=10;
    static const int VERSION_WITH_KEY_ORIGIN = 12;
    static const int CURRENT_VERSION=VERSION_WITH_KEY_ORIGIN;
    int nVersion;
    int64_t nCreateTime;
    std::string hdKeypath;
    CKeyID hd_seed_id;
    KeyOriginInfo key_origin;
    bool has_key_origin = false;
    CKeyMetadata()
    {
        SetNull();
    }
    explicit CKeyMetadata(int64_t nCreateTime_)
    {
        SetNull();
        nCreateTime = nCreateTime_;
    }
    SERIALIZE_METHODS(CKeyMetadata, obj)
    {
        READWRITE(obj.nVersion, obj.nCreateTime);
        if (obj.nVersion >= VERSION_WITH_HDDATA) {
            READWRITE(obj.hdKeypath, obj.hd_seed_id);
        }
        if (obj.nVersion >= VERSION_WITH_KEY_ORIGIN)
        {
            READWRITE(obj.key_origin);
            READWRITE(obj.has_key_origin);
        }
    }
    void SetNull()
    {
        nVersion = CKeyMetadata::CURRENT_VERSION;
        nCreateTime = 0;
        hdKeypath.clear();
        hd_seed_id.SetNull();
        key_origin.clear();
        has_key_origin = false;
    }
};
class WalletBatch
{
private:
    template <typename K, typename T>
    bool WriteIC(const K& key, const T& value, bool fOverwrite = true)
    {
        if (!m_batch->Write(key, value, fOverwrite)) {
            return false;
        }
        m_database.IncrementUpdateCounter();
        if (m_database.nUpdateCounter % 1000 == 0) {
            m_batch->Flush();
        }
        return true;
    }
    template <typename K>
    bool EraseIC(const K& key)
    {
        if (!m_batch->Erase(key)) {
            return false;
        }
        m_database.IncrementUpdateCounter();
        if (m_database.nUpdateCounter % 1000 == 0) {
            m_batch->Flush();
        }
        return true;
    }
public:
    explicit WalletBatch(WalletDatabase &database, bool _fFlushOnClose = true) :
        m_batch(database.MakeBatch(_fFlushOnClose)),
        m_database(database)
    {
    }
    WalletBatch(const WalletBatch&) = delete;
    WalletBatch& operator=(const WalletBatch&) = delete;
    bool WriteName(const std::string& strAddress, const std::string& strName);
    bool EraseName(const std::string& strAddress);
    bool WritePurpose(const std::string& strAddress, const std::string& purpose);
    bool ErasePurpose(const std::string& strAddress);
    bool WriteTx(const CWalletTx& wtx);
    bool EraseTx(uint256 hash);
    bool WriteKeyMetadata(const CKeyMetadata& meta, const CPubKey& pubkey, const bool overwrite);
    bool WriteKey(const CPubKey& vchPubKey, const CPrivKey& vchPrivKey, const CKeyMetadata &keyMeta);
    bool WriteCryptedKey(const CPubKey& vchPubKey, const std::vector<unsigned char>& vchCryptedSecret, const CKeyMetadata &keyMeta);
    bool WriteMasterKey(unsigned int nID, const CMasterKey& kMasterKey);
    bool WriteCScript(const uint160& hash, const CScript& redeemScript);
    bool WriteWatchOnly(const CScript &script, const CKeyMetadata &keymeta);
    bool EraseWatchOnly(const CScript &script);
    bool WriteBestBlock(const CBlockLocator& locator);
    bool ReadBestBlock(CBlockLocator& locator);
    bool WriteOrderPosNext(int64_t nOrderPosNext);
    bool ReadPool(int64_t nPool, CKeyPool& keypool);
    bool WritePool(int64_t nPool, const CKeyPool& keypool);
    bool ErasePool(int64_t nPool);
    bool WriteMinVersion(int nVersion);
    bool WriteDescriptorKey(const uint256& desc_id, const CPubKey& pubkey, const CPrivKey& privkey);
    bool WriteCryptedDescriptorKey(const uint256& desc_id, const CPubKey& pubkey, const std::vector<unsigned char>& secret);
    bool WriteDescriptor(const uint256& desc_id, const WalletDescriptor& descriptor);
    bool WriteDescriptorDerivedCache(const CExtPubKey& xpub, const uint256& desc_id, uint32_t key_exp_index, uint32_t der_index);
    bool WriteDescriptorParentCache(const CExtPubKey& xpub, const uint256& desc_id, uint32_t key_exp_index);
    bool WriteDescriptorLastHardenedCache(const CExtPubKey& xpub, const uint256& desc_id, uint32_t key_exp_index);
    bool WriteDescriptorCacheItems(const uint256& desc_id, const DescriptorCache& cache);
    bool WriteLockedUTXO(const COutPoint& output);
    bool EraseLockedUTXO(const COutPoint& output);
    bool WriteDestData(const std::string &address, const std::string &key, const std::string &value);
    bool EraseDestData(const std::string &address, const std::string &key);
    bool WriteActiveScriptPubKeyMan(uint8_t type, const uint256& id, bool internal);
    bool EraseActiveScriptPubKeyMan(uint8_t type, bool internal);
    DBErrors LoadWallet(CWallet* pwallet);
    DBErrors FindWalletTx(std::vector<uint256>& vTxHash, std::list<CWalletTx>& vWtx);
    DBErrors ZapSelectTx(std::vector<uint256>& vHashIn, std::vector<uint256>& vHashOut);
    static bool IsKeyType(const std::string& strType);
    bool WriteHDChain(const CHDChain& chain);
    bool EraseRecords(const std::unordered_set<std::string>& types);
    bool WriteWalletFlags(const uint64_t flags);
    bool TxnBegin();
    bool TxnCommit();
    bool TxnAbort();
private:
    std::unique_ptr<DatabaseBatch> m_batch;
    WalletDatabase& m_database;
};
void MaybeCompactWalletDB(WalletContext& context);
using KeyFilterFn = std::function<bool(const std::string&)>;
bool ReadKeyValue(CWallet* pwallet, CDataStream& ssKey, CDataStream& ssValue, std::string& strType, std::string& strErr, const KeyFilterFn& filter_fn = nullptr);
std::unique_ptr<WalletDatabase> CreateDummyWalletDatabase();
std::unique_ptr<WalletDatabase> CreateMockWalletDatabase(DatabaseOptions& options);
std::unique_ptr<WalletDatabase> CreateMockWalletDatabase();
}
#endif
