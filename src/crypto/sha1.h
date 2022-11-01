#ifndef LCRYP_CRYPTO_SHA1_H
#define LCRYP_CRYPTO_SHA1_H
#include <cstdlib>
#include <stdint.h>
class CSHA1
{
private:
    uint32_t s[5];
    unsigned char buf[64];
    uint64_t bytes;
public:
    static const size_t OUTPUT_SIZE = 20;
    CSHA1();
    CSHA1& Write(const unsigned char* data, size_t len);
    void Finalize(unsigned char hash[OUTPUT_SIZE]);
    CSHA1& Reset();
};
#endif
