#ifndef LCRYP_CRYPTO_CHACHA20_H
#define LCRYP_CRYPTO_CHACHA20_H
#include <cstdlib>
#include <stdint.h>
class ChaCha20
{
private:
    uint32_t input[16];
public:
    ChaCha20();
    ChaCha20(const unsigned char* key, size_t keylen);
    void SetKey(const unsigned char* key, size_t keylen);
    void SetIV(uint64_t iv);
    void Seek(uint64_t pos);
    void Keystream(unsigned char* c, size_t bytes);
    void Crypt(const unsigned char* input, unsigned char* output, size_t bytes);
};
#endif
