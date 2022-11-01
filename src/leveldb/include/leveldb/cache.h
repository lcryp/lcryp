#ifndef STORAGE_LEVELDB_INCLUDE_CACHE_H_
#define STORAGE_LEVELDB_INCLUDE_CACHE_H_
#include <stdint.h>
#include "leveldb/export.h"
#include "leveldb/slice.h"
namespace leveldb {
class LEVELDB_EXPORT Cache;
LEVELDB_EXPORT Cache* NewLRUCache(size_t capacity);
class LEVELDB_EXPORT Cache {
 public:
  Cache() = default;
  Cache(const Cache&) = delete;
  Cache& operator=(const Cache&) = delete;
  virtual ~Cache();
  struct Handle {};
  virtual Handle* Insert(const Slice& key, void* value, size_t charge,
                         void (*deleter)(const Slice& key, void* value)) = 0;
  virtual Handle* Lookup(const Slice& key) = 0;
  virtual void Release(Handle* handle) = 0;
  virtual void* Value(Handle* handle) = 0;
  virtual void Erase(const Slice& key) = 0;
  virtual uint64_t NewId() = 0;
  virtual void Prune() {}
  virtual size_t TotalCharge() const = 0;
 private:
  void LRU_Remove(Handle* e);
  void LRU_Append(Handle* e);
  void Unref(Handle* e);
  struct Rep;
  Rep* rep_;
};
}
#endif
