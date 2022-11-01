#ifndef SECP256K1_FIELD_H
#define SECP256K1_FIELD_H
#if defined HAVE_CONFIG_H
#include "libsecp256k1-config.h"
#endif
#include "util.h"
#if defined(SECP256K1_WIDEMUL_INT128)
#include "field_5x52.h"
#elif defined(SECP256K1_WIDEMUL_INT64)
#include "field_10x26.h"
#else
#error "Please select wide multiplication implementation"
#endif
static const secp256k1_fe secp256k1_fe_one = SECP256K1_FE_CONST(0, 0, 0, 0, 0, 0, 0, 1);
static const secp256k1_fe secp256k1_const_beta = SECP256K1_FE_CONST(
    0x7ae96a2bul, 0x657c0710ul, 0x6e64479eul, 0xac3434e9ul,
    0x9cf04975ul, 0x12f58995ul, 0xc1396c28ul, 0x719501eeul
);
static void secp256k1_fe_normalize(secp256k1_fe *r);
static void secp256k1_fe_normalize_weak(secp256k1_fe *r);
static void secp256k1_fe_normalize_var(secp256k1_fe *r);
static int secp256k1_fe_normalizes_to_zero(const secp256k1_fe *r);
static int secp256k1_fe_normalizes_to_zero_var(const secp256k1_fe *r);
static void secp256k1_fe_set_int(secp256k1_fe *r, int a);
static void secp256k1_fe_clear(secp256k1_fe *a);
static int secp256k1_fe_is_zero(const secp256k1_fe *a);
static int secp256k1_fe_is_odd(const secp256k1_fe *a);
static int secp256k1_fe_equal(const secp256k1_fe *a, const secp256k1_fe *b);
static int secp256k1_fe_equal_var(const secp256k1_fe *a, const secp256k1_fe *b);
static int secp256k1_fe_cmp_var(const secp256k1_fe *a, const secp256k1_fe *b);
static int secp256k1_fe_set_b32(secp256k1_fe *r, const unsigned char *a);
static void secp256k1_fe_get_b32(unsigned char *r, const secp256k1_fe *a);
static void secp256k1_fe_negate(secp256k1_fe *r, const secp256k1_fe *a, int m);
static void secp256k1_fe_mul_int(secp256k1_fe *r, int a);
static void secp256k1_fe_add(secp256k1_fe *r, const secp256k1_fe *a);
static void secp256k1_fe_mul(secp256k1_fe *r, const secp256k1_fe *a, const secp256k1_fe * SECP256K1_RESTRICT b);
static void secp256k1_fe_sqr(secp256k1_fe *r, const secp256k1_fe *a);
static int secp256k1_fe_sqrt(secp256k1_fe *r, const secp256k1_fe *a);
static void secp256k1_fe_inv(secp256k1_fe *r, const secp256k1_fe *a);
static void secp256k1_fe_inv_var(secp256k1_fe *r, const secp256k1_fe *a);
static void secp256k1_fe_to_storage(secp256k1_fe_storage *r, const secp256k1_fe *a);
static void secp256k1_fe_from_storage(secp256k1_fe *r, const secp256k1_fe_storage *a);
static void secp256k1_fe_storage_cmov(secp256k1_fe_storage *r, const secp256k1_fe_storage *a, int flag);
static void secp256k1_fe_cmov(secp256k1_fe *r, const secp256k1_fe *a, int flag);
static void secp256k1_fe_half(secp256k1_fe *r);
static void secp256k1_fe_get_bounds(secp256k1_fe *r, int m);
#endif
