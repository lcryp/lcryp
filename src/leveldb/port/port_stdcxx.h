#ifndef STORAGE_LEVELDB_PORT_PORT_STDCXX_H_
#define STORAGE_LEVELDB_PORT_PORT_STDCXX_H_
#if defined(LEVELDB_HAS_PORT_CONFIG_H)
#if LEVELDB_HAS_PORT_CONFIG_H
#include "port/port_config.h"
#endif
#elif defined(__has_include)
#if __has_include("port/port_config.h")
#include "port/port_config.h"
#endif
#endif
#if HAVE_CRC32C
#include <crc32c/crc32c.h>
#endif
#if HAVE_SNAPPY
#include <snappy.h>
#endif
#include <cassert>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <string>
#include "port/thread_annotations.h"
namespace leveldb {
namespace port {
static const bool kLittleEndian = !LEVELDB_IS_BIG_ENDIAN;
class CondVar;
class LOCKABLE Mutex {
 public:
  Mutex() = default;
  ~Mutex() = default;
  Mutex(const Mutex&) = delete;
  Mutex& operator=(const Mutex&) = delete;
  void Lock() EXCLUSIVE_LOCK_FUNCTION() { mu_.lock(); }
  void Unlock() UNLOCK_FUNCTION() { mu_.unlock(); }
  void AssertHeld() ASSERT_EXCLUSIVE_LOCK() {}
 private:
  friend class CondVar;
  std::mutex mu_;
};
class CondVar {
 public:
  explicit CondVar(Mutex* mu) : mu_(mu) { assert(mu != nullptr); }
  ~CondVar() = default;
  CondVar(const CondVar&) = delete;
  CondVar& operator=(const CondVar&) = delete;
  void Wait() {
    std::unique_lock<std::mutex> lock(mu_->mu_, std::adopt_lock);
    cv_.wait(lock);
    lock.release();
  }
  void Signal() { cv_.notify_one(); }
  void SignalAll() { cv_.notify_all(); }
 private:
  std::condition_variable cv_;
  Mutex* const mu_;
};
inline bool Snappy_Compress(const char* input, size_t length,
                            std::string* output) {
#if HAVE_SNAPPY
  output->resize(snappy::MaxCompressedLength(length));
  size_t outlen;
  snappy::RawCompress(input, length, &(*output)[0], &outlen);
  output->resize(outlen);
  return true;
#else
  (void)input;
  (void)length;
  (void)output;
#endif
  return false;
}
inline bool Snappy_GetUncompressedLength(const char* input, size_t length,
                                         size_t* result) {
#if HAVE_SNAPPY
  return snappy::GetUncompressedLength(input, length, result);
#else
  (void)input;
  (void)length;
  (void)result;
  return false;
#endif
}
inline bool Snappy_Uncompress(const char* input, size_t length, char* output) {
#if HAVE_SNAPPY
  return snappy::RawUncompress(input, length, output);
#else
  (void)input;
  (void)length;
  (void)output;
  return false;
#endif
}
inline bool GetHeapProfile(void (*func)(void*, const char*, int), void* arg) {
  (void)func;
  (void)arg;
  return false;
}
inline uint32_t AcceleratedCRC32C(uint32_t crc, const char* buf, size_t size) {
#if HAVE_CRC32C
  return ::crc32c::Extend(crc, reinterpret_cast<const uint8_t*>(buf), size);
#else
  (void)crc;
  (void)buf;
  (void)size;
  return 0;
#endif
}
}
}
#endif
