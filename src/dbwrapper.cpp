#include <dbwrapper.h>
#include <fs.h>
#include <logging.h>
#include <random.h>
#include <tinyformat.h>
#include <util/strencodings.h>
#include <util/system.h>
#include <algorithm>
#include <cassert>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <leveldb/cache.h>
#include <leveldb/db.h>
#include <leveldb/env.h>
#include <leveldb/filter_policy.h>
#include <leveldb/helpers/memenv/memenv.h>
#include <leveldb/iterator.h>
#include <leveldb/options.h>
#include <leveldb/status.h>
#include <memory>
#include <optional>
class CLcRypLevelDBLogger : public leveldb::Logger {
public:
    void Logv(const char * format, va_list ap) override {
            if (!LogAcceptCategory(BCLog::LEVELDB, BCLog::Level::Debug)) {
                return;
            }
            char buffer[500];
            for (int iter = 0; iter < 2; iter++) {
                char* base;
                int bufsize;
                if (iter == 0) {
                    bufsize = sizeof(buffer);
                    base = buffer;
                }
                else {
                    bufsize = 30000;
                    base = new char[bufsize];
                }
                char* p = base;
                char* limit = base + bufsize;
                if (p < limit) {
                    va_list backup_ap;
                    va_copy(backup_ap, ap);
                    p += vsnprintf(p, limit - p, format, backup_ap);
                    va_end(backup_ap);
                }
                if (p >= limit) {
                    if (iter == 0) {
                        continue;
                    }
                    else {
                        p = limit - 1;
                    }
                }
                if (p == base || p[-1] != '\n') {
                    *p++ = '\n';
                }
                assert(p <= limit);
                base[std::min(bufsize - 1, (int)(p - base))] = '\0';
                LogPrintLevel(BCLog::LEVELDB, BCLog::Level::Debug, "%s", base);
                if (base != buffer) {
                    delete[] base;
                }
                break;
            }
    }
};
static void SetMaxOpenFiles(leveldb::Options *options) {
    int default_open_files = options->max_open_files;
#ifndef WIN32
    if (sizeof(void*) < 8) {
        options->max_open_files = 64;
    }
#endif
    LogPrint(BCLog::LEVELDB, "LevelDB using max_open_files=%d (default=%d)\n",
             options->max_open_files, default_open_files);
}
static leveldb::Options GetOptions(size_t nCacheSize)
{
    leveldb::Options options;
    options.block_cache = leveldb::NewLRUCache(nCacheSize / 2);
    options.write_buffer_size = nCacheSize / 4;
    options.filter_policy = leveldb::NewBloomFilterPolicy(10);
    options.compression = leveldb::kNoCompression;
    options.info_log = new CLcRypLevelDBLogger();
    if (leveldb::kMajorVersion > 1 || (leveldb::kMajorVersion == 1 && leveldb::kMinorVersion >= 16)) {
        options.paranoid_checks = true;
    }
    SetMaxOpenFiles(&options);
    return options;
}
CDBWrapper::CDBWrapper(const fs::path& path, size_t nCacheSize, bool fMemory, bool fWipe, bool obfuscate)
    : m_name{fs::PathToString(path.stem())}
{
    penv = nullptr;
    readoptions.verify_checksums = true;
    iteroptions.verify_checksums = true;
    iteroptions.fill_cache = false;
    syncoptions.sync = true;
    options = GetOptions(nCacheSize);
    options.create_if_missing = true;
    if (fMemory) {
        penv = leveldb::NewMemEnv(leveldb::Env::Default());
        options.env = penv;
    } else {
        if (fWipe) {
            LogPrintf("Wiping LevelDB in %s\n", fs::PathToString(path));
            leveldb::Status result = leveldb::DestroyDB(fs::PathToString(path), options);
            dbwrapper_private::HandleError(result);
        }
        TryCreateDirectories(path);
        LogPrintf("Opening LevelDB in %s\n", fs::PathToString(path));
    }
    leveldb::Status status = leveldb::DB::Open(options, fs::PathToString(path), &pdb);
    dbwrapper_private::HandleError(status);
    LogPrintf("Opened LevelDB successfully\n");
    if (gArgs.GetBoolArg("-forcecompactdb", false)) {
        LogPrintf("Starting database compaction of %s\n", fs::PathToString(path));
        pdb->CompactRange(nullptr, nullptr);
        LogPrintf("Finished database compaction of %s\n", fs::PathToString(path));
    }
    obfuscate_key = std::vector<unsigned char>(OBFUSCATE_KEY_NUM_BYTES, '\000');
    bool key_exists = Read(OBFUSCATE_KEY_KEY, obfuscate_key);
    if (!key_exists && obfuscate && IsEmpty()) {
        std::vector<unsigned char> new_key = CreateObfuscateKey();
        Write(OBFUSCATE_KEY_KEY, new_key);
        obfuscate_key = new_key;
        LogPrintf("Wrote new obfuscate key for %s: %s\n", fs::PathToString(path), HexStr(obfuscate_key));
    }
    LogPrintf("Using obfuscation key for %s: %s\n", fs::PathToString(path), HexStr(obfuscate_key));
}
CDBWrapper::~CDBWrapper()
{
    delete pdb;
    pdb = nullptr;
    delete options.filter_policy;
    options.filter_policy = nullptr;
    delete options.info_log;
    options.info_log = nullptr;
    delete options.block_cache;
    options.block_cache = nullptr;
    delete penv;
    options.env = nullptr;
}
bool CDBWrapper::WriteBatch(CDBBatch& batch, bool fSync)
{
    const bool log_memory = LogAcceptCategory(BCLog::LEVELDB, BCLog::Level::Debug);
    double mem_before = 0;
    if (log_memory) {
        mem_before = DynamicMemoryUsage() / 1024.0 / 1024;
    }
    leveldb::Status status = pdb->Write(fSync ? syncoptions : writeoptions, &batch.batch);
    dbwrapper_private::HandleError(status);
    if (log_memory) {
        double mem_after = DynamicMemoryUsage() / 1024.0 / 1024;
        LogPrint(BCLog::LEVELDB, "WriteBatch memory usage: db=%s, before=%.1fMiB, after=%.1fMiB\n",
                 m_name, mem_before, mem_after);
    }
    return true;
}
size_t CDBWrapper::DynamicMemoryUsage() const
{
    std::string memory;
    std::optional<size_t> parsed;
    if (!pdb->GetProperty("leveldb.approximate-memory-usage", &memory) || !(parsed = ToIntegral<size_t>(memory))) {
        LogPrint(BCLog::LEVELDB, "Failed to get approximate-memory-usage property\n");
        return 0;
    }
    return parsed.value();
}
const std::string CDBWrapper::OBFUSCATE_KEY_KEY("\000obfuscate_key", 14);
const unsigned int CDBWrapper::OBFUSCATE_KEY_NUM_BYTES = 8;
std::vector<unsigned char> CDBWrapper::CreateObfuscateKey() const
{
    std::vector<uint8_t> ret(OBFUSCATE_KEY_NUM_BYTES);
    GetRandBytes(ret);
    return ret;
}
bool CDBWrapper::IsEmpty()
{
    std::unique_ptr<CDBIterator> it(NewIterator());
    it->SeekToFirst();
    return !(it->Valid());
}
CDBIterator::~CDBIterator() { delete piter; }
bool CDBIterator::Valid() const { return piter->Valid(); }
void CDBIterator::SeekToFirst() { piter->SeekToFirst(); }
void CDBIterator::Next() { piter->Next(); }
namespace dbwrapper_private {
void HandleError(const leveldb::Status& status)
{
    if (status.ok())
        return;
    const std::string errmsg = "Fatal LevelDB error: " + status.ToString();
    LogPrintf("%s\n", errmsg);
    LogPrintf("You can use -debug=leveldb to get more complete diagnostic messages\n");
    throw dbwrapper_error(errmsg);
}
const std::vector<unsigned char>& GetObfuscateKey(const CDBWrapper &w)
{
    return w.obfuscate_key;
}
}
