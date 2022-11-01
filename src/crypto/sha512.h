#ifndef LCRYP_CRYPTO_SHA512_H
#define LCRYP_CRYPTO_SHA512_H
#include <cstdlib>
#include <stdint.h>
class CSHA512
{
private:
    uint64_t s[8];
    unsigned char buf[128];
    uint64_t bytes;
public:
    static constexpr size_t OUTPUT_SIZE = 64;
    CSHA512();
    CSHA512& Write(const unsigned char* data, size_t len);
    void Finalize(unsigned char hash[OUTPUT_SIZE]);
    CSHA512& Reset();
    uint64_t Size() const { return bytes; }
};
#endif
