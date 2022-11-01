#ifndef LCRYP_CRYPTO_RIPEMD160_H
#define LCRYP_CRYPTO_RIPEMD160_H
#include <cstdlib>
#include <stdint.h>
class CRIPEMD160
{
private:
    uint32_t s[5];
    unsigned char buf[64];
    uint64_t bytes;
public:
    static const size_t OUTPUT_SIZE = 20;
    CRIPEMD160();
    CRIPEMD160& Write(const unsigned char* data, size_t len);
    void Finalize(unsigned char hash[OUTPUT_SIZE]);
    CRIPEMD160& Reset();
};
#endif
