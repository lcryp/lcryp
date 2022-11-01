#ifndef LCRYP_WALLET_BDB_H
#define LCRYP_WALLET_BDB_H
#include <clientversion.h>
#include <fs.h>
#include <serialize.h>
#include <streams.h>
#include <util/system.h>
#include <wallet/db.h>
#include <atomic>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsuggest-override"
#endif
#include <db_cxx.h>
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif
struct bilingual_str;
namespace wallet {
struct WalletDatabaseFileId {
    uint8_t value[DB_FILE_ID_LEN];
    bool operator==(const WalletDatabaseFileId& rhs) const;
};
class BerkeleyDatabase;
class BerkeleyEnvironment
{
private:
    bool fDbEnvInit;
    bool fMockDb;
    std::string strPath;
public:
    std::unique_ptr<DbEnv> dbenv;
    std::map<fs::path, std::reference_wrapper<BerkeleyDatabase>> m_databases;
    std::unordered_map<std::string, WalletDatabaseFileId> m_fileids;
    std::condition_variable_any m_db_in_use;
    bool m_use_shared_memory;
    explicit BerkeleyEnvironment(const fs::path& env_directory, bool use_shared_memory);
    BerkeleyEnvironment();
    ~BerkeleyEnvironment();
    void Reset();
    bool IsMock() const { return fMockDb; }
    bool IsInitialized() const { return fDbEnvInit; }
    fs::path Directory() const { return fs::PathFromString(strPath); }
    bool Open(bilingual_str& error);
    void Close();
    void Flush(bool fShutdown);
    void CheckpointLSN(const std::string& strFile);
    void CloseDb(const fs::path& filename);
    void ReloadDbEnv();
    DbTxn* TxnBegin(int flags = DB_TXN_WRITE_NOSYNC)
    {
        DbTxn* ptxn = nullptr;
        int ret = dbenv->txn_begin(nullptr, &ptxn, flags);
        if (!ptxn || ret != 0)
            return nullptr;
        return ptxn;
    }
};
std::shared_ptr<BerkeleyEnvironment> GetBerkeleyEnv(const fs::path& env_directory, bool use_shared_memory);
class BerkeleyBatch;
class BerkeleyDatabase : public WalletDatabase
{
public:
    BerkeleyDatabase() = delete;
    BerkeleyDatabase(std::shared_ptr<BerkeleyEnvironment> env, fs::path filename, const DatabaseOptions& options) :
        WalletDatabase(), env(std::move(env)), m_filename(std::move(filename)), m_max_log_mb(options.max_log_mb)
    {
        auto inserted = this->env->m_databases.emplace(m_filename, std::ref(*this));
        assert(inserted.second);
    }
    ~BerkeleyDatabase() override;
    void Open() override;
    bool Rewrite(const char* pszSkip=nullptr) override;
    void AddRef() override;
    void RemoveRef() override;
    bool Backup(const std::string& strDest) const override;
    void Flush() override;
    void Close() override;
    bool PeriodicFlush() override;
    void IncrementUpdateCounter() override;
    void ReloadDbEnv() override;
    bool Verify(bilingual_str& error);
    std::string Filename() override { return fs::PathToString(env->Directory() / m_filename); }
    std::string Format() override { return "bdb"; }
    std::shared_ptr<BerkeleyEnvironment> env;
    std::unique_ptr<Db> m_db;
    fs::path m_filename;
    int64_t m_max_log_mb;
    std::unique_ptr<DatabaseBatch> MakeBatch(bool flush_on_close = true) override;
};
class BerkeleyBatch : public DatabaseBatch
{
    class SafeDbt final
    {
        Dbt m_dbt;
    public:
        SafeDbt();
        SafeDbt(void* data, size_t size);
        ~SafeDbt();
        const void* get_data() const;
        uint32_t get_size() const;
        operator Dbt*();
    };
private:
    bool ReadKey(CDataStream&& key, CDataStream& value) override;
    bool WriteKey(CDataStream&& key, CDataStream&& value, bool overwrite = true) override;
    bool EraseKey(CDataStream&& key) override;
    bool HasKey(CDataStream&& key) override;
protected:
    Db* pdb;
    std::string strFile;
    DbTxn* activeTxn;
    Dbc* m_cursor;
    bool fReadOnly;
    bool fFlushOnClose;
    BerkeleyEnvironment *env;
    BerkeleyDatabase& m_database;
public:
    explicit BerkeleyBatch(BerkeleyDatabase& database, const bool fReadOnly, bool fFlushOnCloseIn=true);
    ~BerkeleyBatch() override;
    BerkeleyBatch(const BerkeleyBatch&) = delete;
    BerkeleyBatch& operator=(const BerkeleyBatch&) = delete;
    void Flush() override;
    void Close() override;
    bool StartCursor() override;
    bool ReadAtCursor(CDataStream& ssKey, CDataStream& ssValue, bool& complete) override;
    void CloseCursor() override;
    bool TxnBegin() override;
    bool TxnCommit() override;
    bool TxnAbort() override;
};
std::string BerkeleyDatabaseVersion();
bool BerkeleyDatabaseSanityCheck();
std::unique_ptr<BerkeleyDatabase> MakeBerkeleyDatabase(const fs::path& path, const DatabaseOptions& options, DatabaseStatus& status, bilingual_str& error);
}
#endif
