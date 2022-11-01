#ifndef SECP256K1_SCALAR_H
#define SECP256K1_SCALAR_H
#include "util.h"
#if defined HAVE_CONFIG_H
#include "libsecp256k1-config.h"
#endif
#if defined(EXHAUSTIVE_TEST_ORDER)
#include "scalar_low.h"
#elif defined(SECP256K1_WIDEMUL_INT128)
#include "scalar_4x64.h"
#elif defined(SECP256K1_WIDEMUL_INT64)
#include "scalar_8x32.h"
#else
#error "Please select wide multiplication implementation"
#endif
static void secp256k1_scalar_clear(secp256k1_scalar *r);
static unsigned int secp256k1_scalar_get_bits(const secp256k1_scalar *a, unsigned int offset, unsigned int count);
static unsigned int secp256k1_scalar_get_bits_var(const secp256k1_scalar *a, unsigned int offset, unsigned int count);
static void secp256k1_scalar_set_b32(secp256k1_scalar *r, const unsigned char *bin, int *overflow);
static int secp256k1_scalar_set_b32_seckey(secp256k1_scalar *r, const unsigned char *bin);
static void secp256k1_scalar_set_int(secp256k1_scalar *r, unsigned int v);
static void secp256k1_scalar_get_b32(unsigned char *bin, const secp256k1_scalar* a);
static int secp256k1_scalar_add(secp256k1_scalar *r, const secp256k1_scalar *a, const secp256k1_scalar *b);
static void secp256k1_scalar_cadd_bit(secp256k1_scalar *r, unsigned int bit, int flag);
static void secp256k1_scalar_mul(secp256k1_scalar *r, const secp256k1_scalar *a, const secp256k1_scalar *b);
static int secp256k1_scalar_shr_int(secp256k1_scalar *r, int n);
static void secp256k1_scalar_inverse(secp256k1_scalar *r, const secp256k1_scalar *a);
static void secp256k1_scalar_inverse_var(secp256k1_scalar *r, const secp256k1_scalar *a);
static void secp256k1_scalar_negate(secp256k1_scalar *r, const secp256k1_scalar *a);
static int secp256k1_scalar_is_zero(const secp256k1_scalar *a);
static int secp256k1_scalar_is_one(const secp256k1_scalar *a);
static int secp256k1_scalar_is_even(const secp256k1_scalar *a);
static int secp256k1_scalar_is_high(const secp256k1_scalar *a);
static int secp256k1_scalar_cond_negate(secp256k1_scalar *a, int flag);
static int secp256k1_scalar_eq(const secp256k1_scalar *a, const secp256k1_scalar *b);
static void secp256k1_scalar_split_128(secp256k1_scalar *r1, secp256k1_scalar *r2, const secp256k1_scalar *k);
static void secp256k1_scalar_split_lambda(secp256k1_scalar *r1, secp256k1_scalar *r2, const secp256k1_scalar *k);
static void secp256k1_scalar_mul_shift_var(secp256k1_scalar *r, const secp256k1_scalar *a, const secp256k1_scalar *b, unsigned int shift);
static void secp256k1_scalar_cmov(secp256k1_scalar *r, const secp256k1_scalar *a, int flag);
#endif
