#ifndef LCRYP_CRYPTO_POLY1305_H
#define LCRYP_CRYPTO_POLY1305_H
#include <cstdlib>
#include <stdint.h>
#define POLY1305_KEYLEN 32
#define POLY1305_TAGLEN 16
void poly1305_auth(unsigned char out[POLY1305_TAGLEN], const unsigned char *m, size_t inlen,
    const unsigned char key[POLY1305_KEYLEN]);
#endif
