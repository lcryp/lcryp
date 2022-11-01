#ifndef LCRYP_CRYPTO_CHACHA_POLY_AEAD_H
#define LCRYP_CRYPTO_CHACHA_POLY_AEAD_H
#include <crypto/chacha20.h>
#include <cmath>
static constexpr int CHACHA20_POLY1305_AEAD_KEY_LEN = 32;
static constexpr int CHACHA20_POLY1305_AEAD_AAD_LEN = 3;
static constexpr int CHACHA20_ROUND_OUTPUT = 64;
static constexpr int AAD_PACKAGES_PER_ROUND = 21;
class ChaCha20Poly1305AEAD
{
private:
    ChaCha20 m_chacha_header;
    ChaCha20 m_chacha_main;
    unsigned char m_aad_keystream_buffer[CHACHA20_ROUND_OUTPUT];
    uint64_t m_cached_aad_seqnr;
public:
    ChaCha20Poly1305AEAD(const unsigned char* K_1, size_t K_1_len, const unsigned char* K_2, size_t K_2_len);
    explicit ChaCha20Poly1305AEAD(const ChaCha20Poly1305AEAD&) = delete;
    bool Crypt(uint64_t seqnr_payload, uint64_t seqnr_aad, int aad_pos, unsigned char* dest, size_t dest_len, const unsigned char* src, size_t src_len, bool is_encrypt);
    bool GetLength(uint32_t* len24_out, uint64_t seqnr_aad, int aad_pos, const uint8_t* ciphertext);
};
#endif
