#ifndef LCRYP_CRYPTO_SIPHASH_H
#define LCRYP_CRYPTO_SIPHASH_H
#include <stdint.h>
#include <uint256.h>
class CSipHasher
{
private:
    uint64_t v[4];
    uint64_t tmp;
    uint8_t count;
public:
    CSipHasher(uint64_t k0, uint64_t k1);
    CSipHasher& Write(uint64_t data);
    CSipHasher& Write(const unsigned char* data, size_t size);
    uint64_t Finalize() const;
};
uint64_t SipHashUint256(uint64_t k0, uint64_t k1, const uint256& val);
uint64_t SipHashUint256Extra(uint64_t k0, uint64_t k1, const uint256& val, uint32_t extra);
#endif
