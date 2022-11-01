#ifndef LCRYP_DBWRAPPER_H
#define LCRYP_DBWRAPPER_H
#include <clientversion.h>
#include <fs.h>
#include <logging.h>
#include <serialize.h>
#include <span.h>
#include <streams.h>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <leveldb/db.h>
#include <leveldb/iterator.h>
#include <leveldb/options.h>
#include <leveldb/slice.h>
#include <leveldb/status.h>
#include <leveldb/write_batch.h>
#include <stdexcept>
#include <string>
#include <vector>
namespace leveldb {
class Env;
}
static const size_t DBWRAPPER_PREALLOC_KEY_SIZE = 64;
static const size_t DBWRAPPER_PREALLOC_VALUE_SIZE = 1024;
class dbwrapper_error : public std::runtime_error
{
public:
    explicit dbwrapper_error(const std::string& msg) : std::runtime_error(msg) {}
};
class CDBWrapper;
namespace dbwrapper_private {
void HandleError(const leveldb::Status& status);
const std::vector<unsigned char>& GetObfuscateKey(const CDBWrapper &w);
};
class CDBBatch
{
    friend class CDBWrapper;
private:
    const CDBWrapper &parent;
    leveldb::WriteBatch batch;
    CDataStream ssKey;
    CDataStream ssValue;
    size_t size_estimate;
public:
    explicit CDBBatch(const CDBWrapper &_parent) : parent(_parent), ssKey(SER_DISK, CLIENT_VERSION), ssValue(SER_DISK, CLIENT_VERSION), size_estimate(0) { };
    void Clear()
    {
        batch.Clear();
        size_estimate = 0;
    }
    template <typename K, typename V>
    void Write(const K& key, const V& value)
    {
        ssKey.reserve(DBWRAPPER_PREALLOC_KEY_SIZE);
        ssKey << key;
        leveldb::Slice slKey((const char*)ssKey.data(), ssKey.size());
        ssValue.reserve(DBWRAPPER_PREALLOC_VALUE_SIZE);
        ssValue << value;
        ssValue.Xor(dbwrapper_private::GetObfuscateKey(parent));
        leveldb::Slice slValue((const char*)ssValue.data(), ssValue.size());
        batch.Put(slKey, slValue);
        size_estimate += 3 + (slKey.size() > 127) + slKey.size() + (slValue.size() > 127) + slValue.size();
        ssKey.clear();
        ssValue.clear();
    }
    template <typename K>
    void Erase(const K& key)
    {
        ssKey.reserve(DBWRAPPER_PREALLOC_KEY_SIZE);
        ssKey << key;
        leveldb::Slice slKey((const char*)ssKey.data(), ssKey.size());
        batch.Delete(slKey);
        size_estimate += 2 + (slKey.size() > 127) + slKey.size();
        ssKey.clear();
    }
    size_t SizeEstimate() const { return size_estimate; }
};
class CDBIterator
{
private:
    const CDBWrapper &parent;
    leveldb::Iterator *piter;
public:
    CDBIterator(const CDBWrapper &_parent, leveldb::Iterator *_piter) :
        parent(_parent), piter(_piter) { };
    ~CDBIterator();
    bool Valid() const;
    void SeekToFirst();
    template<typename K> void Seek(const K& key) {
        CDataStream ssKey(SER_DISK, CLIENT_VERSION);
        ssKey.reserve(DBWRAPPER_PREALLOC_KEY_SIZE);
        ssKey << key;
        leveldb::Slice slKey((const char*)ssKey.data(), ssKey.size());
        piter->Seek(slKey);
    }
    void Next();
    template<typename K> bool GetKey(K& key) {
        leveldb::Slice slKey = piter->key();
        try {
            CDataStream ssKey{MakeByteSpan(slKey), SER_DISK, CLIENT_VERSION};
            ssKey >> key;
        } catch (const std::exception&) {
            return false;
        }
        return true;
    }
    template<typename V> bool GetValue(V& value) {
        leveldb::Slice slValue = piter->value();
        try {
            CDataStream ssValue{MakeByteSpan(slValue), SER_DISK, CLIENT_VERSION};
            ssValue.Xor(dbwrapper_private::GetObfuscateKey(parent));
            ssValue >> value;
        } catch (const std::exception&) {
            return false;
        }
        return true;
    }
};
class CDBWrapper
{
    friend const std::vector<unsigned char>& dbwrapper_private::GetObfuscateKey(const CDBWrapper &w);
private:
    leveldb::Env* penv;
    leveldb::Options options;
    leveldb::ReadOptions readoptions;
    leveldb::ReadOptions iteroptions;
    leveldb::WriteOptions writeoptions;
    leveldb::WriteOptions syncoptions;
    leveldb::DB* pdb;
    std::string m_name;
    std::vector<unsigned char> obfuscate_key;
    static const std::string OBFUSCATE_KEY_KEY;
    static const unsigned int OBFUSCATE_KEY_NUM_BYTES;
    std::vector<unsigned char> CreateObfuscateKey() const;
public:
    CDBWrapper(const fs::path& path, size_t nCacheSize, bool fMemory = false, bool fWipe = false, bool obfuscate = false);
    ~CDBWrapper();
    CDBWrapper(const CDBWrapper&) = delete;
    CDBWrapper& operator=(const CDBWrapper&) = delete;
    template <typename K, typename V>
    bool Read(const K& key, V& value) const
    {
        CDataStream ssKey(SER_DISK, CLIENT_VERSION);
        ssKey.reserve(DBWRAPPER_PREALLOC_KEY_SIZE);
        ssKey << key;
        leveldb::Slice slKey((const char*)ssKey.data(), ssKey.size());
        std::string strValue;
        leveldb::Status status = pdb->Get(readoptions, slKey, &strValue);
        if (!status.ok()) {
            if (status.IsNotFound())
                return false;
            LogPrintf("LevelDB read failure: %s\n", status.ToString());
            dbwrapper_private::HandleError(status);
        }
        try {
            CDataStream ssValue{MakeByteSpan(strValue), SER_DISK, CLIENT_VERSION};
            ssValue.Xor(obfuscate_key);
            ssValue >> value;
        } catch (const std::exception&) {
            return false;
        }
        return true;
    }
    template <typename K, typename V>
    bool Write(const K& key, const V& value, bool fSync = false)
    {
        CDBBatch batch(*this);
        batch.Write(key, value);
        return WriteBatch(batch, fSync);
    }
    template <typename K>
    bool Exists(const K& key) const
    {
        CDataStream ssKey(SER_DISK, CLIENT_VERSION);
        ssKey.reserve(DBWRAPPER_PREALLOC_KEY_SIZE);
        ssKey << key;
        leveldb::Slice slKey((const char*)ssKey.data(), ssKey.size());
        std::string strValue;
        leveldb::Status status = pdb->Get(readoptions, slKey, &strValue);
        if (!status.ok()) {
            if (status.IsNotFound())
                return false;
            LogPrintf("LevelDB read failure: %s\n", status.ToString());
            dbwrapper_private::HandleError(status);
        }
        return true;
    }
    template <typename K>
    bool Erase(const K& key, bool fSync = false)
    {
        CDBBatch batch(*this);
        batch.Erase(key);
        return WriteBatch(batch, fSync);
    }
    bool WriteBatch(CDBBatch& batch, bool fSync = false);
    size_t DynamicMemoryUsage() const;
    CDBIterator *NewIterator()
    {
        return new CDBIterator(*this, pdb->NewIterator(iteroptions));
    }
    bool IsEmpty();
    template<typename K>
    size_t EstimateSize(const K& key_begin, const K& key_end) const
    {
        CDataStream ssKey1(SER_DISK, CLIENT_VERSION), ssKey2(SER_DISK, CLIENT_VERSION);
        ssKey1.reserve(DBWRAPPER_PREALLOC_KEY_SIZE);
        ssKey2.reserve(DBWRAPPER_PREALLOC_KEY_SIZE);
        ssKey1 << key_begin;
        ssKey2 << key_end;
        leveldb::Slice slKey1((const char*)ssKey1.data(), ssKey1.size());
        leveldb::Slice slKey2((const char*)ssKey2.data(), ssKey2.size());
        uint64_t size = 0;
        leveldb::Range range(slKey1, slKey2);
        pdb->GetApproximateSizes(&range, 1, &size);
        return size;
    }
};
#endif
