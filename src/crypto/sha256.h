#ifndef LCRYP_CRYPTO_SHA256_H
#define LCRYP_CRYPTO_SHA256_H
#include <cstdlib>
#include <stdint.h>
#include <string>
class CSHA256
{
private:
    uint32_t s[8];
    unsigned char buf[64];
    uint64_t bytes;
public:
    static const size_t OUTPUT_SIZE = 32;
    CSHA256();
    CSHA256& Write(const unsigned char* data, size_t len);
    void Finalize(unsigned char hash[OUTPUT_SIZE]);
    CSHA256& Reset();
};
std::string SHA256AutoDetect();
void SHA256D64(unsigned char* output, const unsigned char* input, size_t blocks);
#endif
