#ifndef SECP256K1_ECMULT_CONST_IMPL_H
#define SECP256K1_ECMULT_CONST_IMPL_H
#include "scalar.h"
#include "group.h"
#include "ecmult_const.h"
#include "ecmult_impl.h"
static void secp256k1_ecmult_odd_multiples_table_globalz_windowa(secp256k1_ge *pre, secp256k1_fe *globalz, const secp256k1_gej *a) {
    secp256k1_fe zr[ECMULT_TABLE_SIZE(WINDOW_A)];
    secp256k1_ecmult_odd_multiples_table(ECMULT_TABLE_SIZE(WINDOW_A), pre, zr, globalz, a);
    secp256k1_ge_table_set_globalz(ECMULT_TABLE_SIZE(WINDOW_A), pre, zr);
}
#define ECMULT_CONST_TABLE_GET_GE(r,pre,n,w) do { \
    int m = 0; \
      \
    int mask = (n) >> (sizeof(n) * CHAR_BIT - 1); \
    int abs_n = ((n) + mask) ^ mask; \
    int idx_n = abs_n >> 1; \
    secp256k1_fe neg_y; \
    VERIFY_CHECK(((n) & 1) == 1); \
    VERIFY_CHECK((n) >= -((1 << ((w)-1)) - 1)); \
    VERIFY_CHECK((n) <=  ((1 << ((w)-1)) - 1)); \
    VERIFY_SETUP(secp256k1_fe_clear(&(r)->x)); \
    VERIFY_SETUP(secp256k1_fe_clear(&(r)->y)); \
      \
    (r)->x = (pre)[m].x; \
    (r)->y = (pre)[m].y; \
    for (m = 1; m < ECMULT_TABLE_SIZE(w); m++) { \
          \
        secp256k1_fe_cmov(&(r)->x, &(pre)[m].x, m == idx_n); \
        secp256k1_fe_cmov(&(r)->y, &(pre)[m].y, m == idx_n); \
    } \
    (r)->infinity = 0; \
    secp256k1_fe_negate(&neg_y, &(r)->y, 1); \
    secp256k1_fe_cmov(&(r)->y, &neg_y, (n) != abs_n); \
} while(0)
static int secp256k1_wnaf_const(int *wnaf, const secp256k1_scalar *scalar, int w, int size) {
    int global_sign;
    int skew;
    int word = 0;
    int u_last;
    int u;
    int flip;
    secp256k1_scalar s = *scalar;
    VERIFY_CHECK(w > 0);
    VERIFY_CHECK(size > 0);
    flip = secp256k1_scalar_is_high(&s);
    skew = flip ^ secp256k1_scalar_is_even(&s);
    secp256k1_scalar_cadd_bit(&s, 0, skew);
    global_sign = secp256k1_scalar_cond_negate(&s, flip);
    u_last = secp256k1_scalar_shr_int(&s, w);
    do {
        int even;
        u = secp256k1_scalar_shr_int(&s, w);
        even = ((u & 1) == 0);
        VERIFY_CHECK(u_last > 0);
        VERIFY_CHECK((u_last & 1) == 1);
        u += even;
        u_last -= even * (1 << w);
        wnaf[word++] = u_last * global_sign;
        u_last = u;
    } while (word * w < size);
    wnaf[word] = u * global_sign;
    VERIFY_CHECK(secp256k1_scalar_is_zero(&s));
    VERIFY_CHECK(word == WNAF_SIZE_BITS(size, w));
    return skew;
}
static void secp256k1_ecmult_const(secp256k1_gej *r, const secp256k1_ge *a, const secp256k1_scalar *scalar, int size) {
    secp256k1_ge pre_a[ECMULT_TABLE_SIZE(WINDOW_A)];
    secp256k1_ge tmpa;
    secp256k1_fe Z;
    int skew_1;
    secp256k1_ge pre_a_lam[ECMULT_TABLE_SIZE(WINDOW_A)];
    int wnaf_lam[1 + WNAF_SIZE(WINDOW_A - 1)];
    int skew_lam;
    secp256k1_scalar q_1, q_lam;
    int wnaf_1[1 + WNAF_SIZE(WINDOW_A - 1)];
    int i;
    int rsize = size;
    if (size > 128) {
        rsize = 128;
        secp256k1_scalar_split_lambda(&q_1, &q_lam, scalar);
        skew_1   = secp256k1_wnaf_const(wnaf_1,   &q_1,   WINDOW_A - 1, 128);
        skew_lam = secp256k1_wnaf_const(wnaf_lam, &q_lam, WINDOW_A - 1, 128);
    } else
    {
        skew_1   = secp256k1_wnaf_const(wnaf_1, scalar, WINDOW_A - 1, size);
        skew_lam = 0;
    }
    VERIFY_CHECK(!a->infinity);
    secp256k1_gej_set_ge(r, a);
    secp256k1_ecmult_odd_multiples_table_globalz_windowa(pre_a, &Z, r);
    for (i = 0; i < ECMULT_TABLE_SIZE(WINDOW_A); i++) {
        secp256k1_fe_normalize_weak(&pre_a[i].y);
    }
    if (size > 128) {
        for (i = 0; i < ECMULT_TABLE_SIZE(WINDOW_A); i++) {
            secp256k1_ge_mul_lambda(&pre_a_lam[i], &pre_a[i]);
        }
    }
    i = wnaf_1[WNAF_SIZE_BITS(rsize, WINDOW_A - 1)];
    VERIFY_CHECK(i != 0);
    ECMULT_CONST_TABLE_GET_GE(&tmpa, pre_a, i, WINDOW_A);
    secp256k1_gej_set_ge(r, &tmpa);
    if (size > 128) {
        i = wnaf_lam[WNAF_SIZE_BITS(rsize, WINDOW_A - 1)];
        VERIFY_CHECK(i != 0);
        ECMULT_CONST_TABLE_GET_GE(&tmpa, pre_a_lam, i, WINDOW_A);
        secp256k1_gej_add_ge(r, r, &tmpa);
    }
    for (i = WNAF_SIZE_BITS(rsize, WINDOW_A - 1) - 1; i >= 0; i--) {
        int n;
        int j;
        for (j = 0; j < WINDOW_A - 1; ++j) {
            secp256k1_gej_double(r, r);
        }
        n = wnaf_1[i];
        ECMULT_CONST_TABLE_GET_GE(&tmpa, pre_a, n, WINDOW_A);
        VERIFY_CHECK(n != 0);
        secp256k1_gej_add_ge(r, r, &tmpa);
        if (size > 128) {
            n = wnaf_lam[i];
            ECMULT_CONST_TABLE_GET_GE(&tmpa, pre_a_lam, n, WINDOW_A);
            VERIFY_CHECK(n != 0);
            secp256k1_gej_add_ge(r, r, &tmpa);
        }
    }
    {
        secp256k1_gej tmpj;
        secp256k1_ge_neg(&tmpa, &pre_a[0]);
        secp256k1_gej_add_ge(&tmpj, r, &tmpa);
        secp256k1_gej_cmov(r, &tmpj, skew_1);
        if (size > 128) {
            secp256k1_ge_neg(&tmpa, &pre_a_lam[0]);
            secp256k1_gej_add_ge(&tmpj, r, &tmpa);
            secp256k1_gej_cmov(r, &tmpj, skew_lam);
        }
    }
    secp256k1_fe_mul(&r->z, &r->z, &Z);
}
#endif
