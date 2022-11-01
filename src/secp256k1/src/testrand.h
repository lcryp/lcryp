#ifndef SECP256K1_TESTRAND_H
#define SECP256K1_TESTRAND_H
#if defined HAVE_CONFIG_H
#include "libsecp256k1-config.h"
#endif
SECP256K1_INLINE static void secp256k1_testrand_seed(const unsigned char *seed16);
SECP256K1_INLINE static uint32_t secp256k1_testrand32(void);
SECP256K1_INLINE static uint64_t secp256k1_testrand64(void);
SECP256K1_INLINE static uint64_t secp256k1_testrand_bits(int bits);
static uint32_t secp256k1_testrand_int(uint32_t range);
static void secp256k1_testrand256(unsigned char *b32);
static void secp256k1_testrand256_test(unsigned char *b32);
static void secp256k1_testrand_bytes_test(unsigned char *bytes, size_t len);
static void secp256k1_testrand_flip(unsigned char *b, size_t len);
static void secp256k1_testrand_init(const char* hexseed);
static void secp256k1_testrand_finish(void);
#endif
