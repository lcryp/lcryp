#ifndef LCRYP_WALLET_SQLITE_H
#define LCRYP_WALLET_SQLITE_H
#include <wallet/db.h>
#include <sqlite3.h>
struct bilingual_str;
namespace wallet {
class SQLiteDatabase;
class SQLiteBatch : public DatabaseBatch
{
private:
    SQLiteDatabase& m_database;
    bool m_cursor_init = false;
    sqlite3_stmt* m_read_stmt{nullptr};
    sqlite3_stmt* m_insert_stmt{nullptr};
    sqlite3_stmt* m_overwrite_stmt{nullptr};
    sqlite3_stmt* m_delete_stmt{nullptr};
    sqlite3_stmt* m_cursor_stmt{nullptr};
    void SetupSQLStatements();
    bool ReadKey(CDataStream&& key, CDataStream& value) override;
    bool WriteKey(CDataStream&& key, CDataStream&& value, bool overwrite = true) override;
    bool EraseKey(CDataStream&& key) override;
    bool HasKey(CDataStream&& key) override;
public:
    explicit SQLiteBatch(SQLiteDatabase& database);
    ~SQLiteBatch() override { Close(); }
    void Flush() override {}
    void Close() override;
    bool StartCursor() override;
    bool ReadAtCursor(CDataStream& key, CDataStream& value, bool& complete) override;
    void CloseCursor() override;
    bool TxnBegin() override;
    bool TxnCommit() override;
    bool TxnAbort() override;
};
class SQLiteDatabase : public WalletDatabase
{
private:
    const bool m_mock{false};
    const std::string m_dir_path;
    const std::string m_file_path;
    void Cleanup() noexcept;
public:
    SQLiteDatabase() = delete;
    SQLiteDatabase(const fs::path& dir_path, const fs::path& file_path, const DatabaseOptions& options, bool mock = false);
    ~SQLiteDatabase();
    bool Verify(bilingual_str& error);
    void Open() override;
    void Close() override;
    void AddRef() override { assert(false); }
    void RemoveRef() override { assert(false); }
    bool Rewrite(const char* skip = nullptr) override;
    bool Backup(const std::string& dest) const override;
    void Flush() override {}
    bool PeriodicFlush() override { return false; }
    void ReloadDbEnv() override {}
    void IncrementUpdateCounter() override { ++nUpdateCounter; }
    std::string Filename() override { return m_file_path; }
    std::string Format() override { return "sqlite"; }
    std::unique_ptr<DatabaseBatch> MakeBatch(bool flush_on_close = true) override;
    sqlite3* m_db{nullptr};
    bool m_use_unsafe_sync;
};
std::unique_ptr<SQLiteDatabase> MakeSQLiteDatabase(const fs::path& path, const DatabaseOptions& options, DatabaseStatus& status, bilingual_str& error);
std::string SQLiteDatabaseVersion();
}
#endif
