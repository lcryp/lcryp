#if defined(HAVE_CONFIG_H)
#include <config/lcryp-config.h>
#endif
#include <crypto/chacha_poly_aead.h>
#include <crypto/poly1305.h>
#include <support/cleanse.h>
#include <assert.h>
#include <string.h>
#include <cstdio>
#include <limits>
#ifndef HAVE_TIMINGSAFE_BCMP
int timingsafe_bcmp(const unsigned char* b1, const unsigned char* b2, size_t n)
{
    const unsigned char *p1 = b1, *p2 = b2;
    int ret = 0;
    for (; n > 0; n--)
        ret |= *p1++ ^ *p2++;
    return (ret != 0);
}
#endif
ChaCha20Poly1305AEAD::ChaCha20Poly1305AEAD(const unsigned char* K_1, size_t K_1_len, const unsigned char* K_2, size_t K_2_len)
{
    assert(K_1_len == CHACHA20_POLY1305_AEAD_KEY_LEN);
    assert(K_2_len == CHACHA20_POLY1305_AEAD_KEY_LEN);
    m_chacha_header.SetKey(K_1, CHACHA20_POLY1305_AEAD_KEY_LEN);
    m_chacha_main.SetKey(K_2, CHACHA20_POLY1305_AEAD_KEY_LEN);
    m_cached_aad_seqnr = std::numeric_limits<uint64_t>::max();
}
bool ChaCha20Poly1305AEAD::Crypt(uint64_t seqnr_payload, uint64_t seqnr_aad, int aad_pos, unsigned char* dest, size_t dest_len  , const unsigned char* src, size_t src_len, bool is_encrypt)
{
    if (
        (is_encrypt && (src_len < CHACHA20_POLY1305_AEAD_AAD_LEN || dest_len < src_len + POLY1305_TAGLEN)) ||
        (!is_encrypt && (src_len < CHACHA20_POLY1305_AEAD_AAD_LEN + POLY1305_TAGLEN || dest_len < src_len - POLY1305_TAGLEN))) {
        return false;
    }
    unsigned char expected_tag[POLY1305_TAGLEN], poly_key[POLY1305_KEYLEN];
    memset(poly_key, 0, sizeof(poly_key));
    m_chacha_main.SetIV(seqnr_payload);
    m_chacha_main.Seek(0);
    m_chacha_main.Crypt(poly_key, poly_key, sizeof(poly_key));
    if (!is_encrypt) {
        const unsigned char* tag = src + src_len - POLY1305_TAGLEN;
        poly1305_auth(expected_tag, src, src_len - POLY1305_TAGLEN, poly_key);
        if (timingsafe_bcmp(expected_tag, tag, POLY1305_TAGLEN) != 0) {
            memory_cleanse(expected_tag, sizeof(expected_tag));
            memory_cleanse(poly_key, sizeof(poly_key));
            return false;
        }
        memory_cleanse(expected_tag, sizeof(expected_tag));
        src_len -= POLY1305_TAGLEN;
    }
    if (m_cached_aad_seqnr != seqnr_aad) {
        m_cached_aad_seqnr = seqnr_aad;
        m_chacha_header.SetIV(seqnr_aad);
        m_chacha_header.Seek(0);
        m_chacha_header.Keystream(m_aad_keystream_buffer, CHACHA20_ROUND_OUTPUT);
    }
    dest[0] = src[0] ^ m_aad_keystream_buffer[aad_pos];
    dest[1] = src[1] ^ m_aad_keystream_buffer[aad_pos + 1];
    dest[2] = src[2] ^ m_aad_keystream_buffer[aad_pos + 2];
    m_chacha_main.Seek(1);
    m_chacha_main.Crypt(src + CHACHA20_POLY1305_AEAD_AAD_LEN, dest + CHACHA20_POLY1305_AEAD_AAD_LEN, src_len - CHACHA20_POLY1305_AEAD_AAD_LEN);
    if (is_encrypt) {
        poly1305_auth(dest + src_len, dest, src_len, poly_key);
    }
    memory_cleanse(poly_key, sizeof(poly_key));
    return true;
}
bool ChaCha20Poly1305AEAD::GetLength(uint32_t* len24_out, uint64_t seqnr_aad, int aad_pos, const uint8_t* ciphertext)
{
    assert(aad_pos >= 0 && aad_pos < CHACHA20_ROUND_OUTPUT - CHACHA20_POLY1305_AEAD_AAD_LEN);
    if (m_cached_aad_seqnr != seqnr_aad) {
        m_cached_aad_seqnr = seqnr_aad;
        m_chacha_header.SetIV(seqnr_aad);
        m_chacha_header.Seek(0);
        m_chacha_header.Keystream(m_aad_keystream_buffer, CHACHA20_ROUND_OUTPUT);
    }
    *len24_out = (ciphertext[0] ^ m_aad_keystream_buffer[aad_pos + 0]) |
                 (ciphertext[1] ^ m_aad_keystream_buffer[aad_pos + 1]) << 8 |
                 (ciphertext[2] ^ m_aad_keystream_buffer[aad_pos + 2]) << 16;
    return true;
}
