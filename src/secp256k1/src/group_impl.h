#ifndef SECP256K1_GROUP_IMPL_H
#define SECP256K1_GROUP_IMPL_H
#include "field.h"
#include "group.h"
#define SECP256K1_G_ORDER_13 SECP256K1_GE_CONST(\
    0xc3459c3d, 0x35326167, 0xcd86cce8, 0x07a2417f,\
    0x5b8bd567, 0xde8538ee, 0x0d507b0c, 0xd128f5bb,\
    0x8e467fec, 0xcd30000a, 0x6cc1184e, 0x25d382c2,\
    0xa2f4494e, 0x2fbe9abc, 0x8b64abac, 0xd005fb24\
)
#define SECP256K1_G_ORDER_199 SECP256K1_GE_CONST(\
    0x226e653f, 0xc8df7744, 0x9bacbf12, 0x7d1dcbf9,\
    0x87f05b2a, 0xe7edbd28, 0x1f564575, 0xc48dcf18,\
    0xa13872c2, 0xe933bb17, 0x5d9ffd5b, 0xb5b6e10c,\
    0x57fe3c00, 0xbaaaa15a, 0xe003ec3e, 0x9c269bae\
)
#define SECP256K1_G SECP256K1_GE_CONST(\
    0x79BE667EUL, 0xF9DCBBACUL, 0x55A06295UL, 0xCE870B07UL,\
    0x029BFCDBUL, 0x2DCE28D9UL, 0x59F2815BUL, 0x16F81798UL,\
    0x483ADA77UL, 0x26A3C465UL, 0x5DA4FBFCUL, 0x0E1108A8UL,\
    0xFD17B448UL, 0xA6855419UL, 0x9C47D08FUL, 0xFB10D4B8UL\
)
#if defined(EXHAUSTIVE_TEST_ORDER)
#  if EXHAUSTIVE_TEST_ORDER == 13
static const secp256k1_ge secp256k1_ge_const_g = SECP256K1_G_ORDER_13;
static const secp256k1_fe secp256k1_fe_const_b = SECP256K1_FE_CONST(
    0x3d3486b2, 0x159a9ca5, 0xc75638be, 0xb23a69bc,
    0x946a45ab, 0x24801247, 0xb4ed2b8e, 0x26b6a417
);
#  elif EXHAUSTIVE_TEST_ORDER == 199
static const secp256k1_ge secp256k1_ge_const_g = SECP256K1_G_ORDER_199;
static const secp256k1_fe secp256k1_fe_const_b = SECP256K1_FE_CONST(
    0x2cca28fa, 0xfc614b80, 0x2a3db42b, 0x00ba00b1,
    0xbea8d943, 0xdace9ab2, 0x9536daea, 0x0074defb
);
#  else
#    error No known generator for the specified exhaustive test group order.
#  endif
#else
static const secp256k1_ge secp256k1_ge_const_g = SECP256K1_G;
static const secp256k1_fe secp256k1_fe_const_b = SECP256K1_FE_CONST(0, 0, 0, 0, 0, 0, 0, 7);
#endif
static void secp256k1_ge_set_gej_zinv(secp256k1_ge *r, const secp256k1_gej *a, const secp256k1_fe *zi) {
    secp256k1_fe zi2;
    secp256k1_fe zi3;
    VERIFY_CHECK(!a->infinity);
    secp256k1_fe_sqr(&zi2, zi);
    secp256k1_fe_mul(&zi3, &zi2, zi);
    secp256k1_fe_mul(&r->x, &a->x, &zi2);
    secp256k1_fe_mul(&r->y, &a->y, &zi3);
    r->infinity = a->infinity;
}
static void secp256k1_ge_set_xy(secp256k1_ge *r, const secp256k1_fe *x, const secp256k1_fe *y) {
    r->infinity = 0;
    r->x = *x;
    r->y = *y;
}
static int secp256k1_ge_is_infinity(const secp256k1_ge *a) {
    return a->infinity;
}
static void secp256k1_ge_neg(secp256k1_ge *r, const secp256k1_ge *a) {
    *r = *a;
    secp256k1_fe_normalize_weak(&r->y);
    secp256k1_fe_negate(&r->y, &r->y, 1);
}
static void secp256k1_ge_set_gej(secp256k1_ge *r, secp256k1_gej *a) {
    secp256k1_fe z2, z3;
    r->infinity = a->infinity;
    secp256k1_fe_inv(&a->z, &a->z);
    secp256k1_fe_sqr(&z2, &a->z);
    secp256k1_fe_mul(&z3, &a->z, &z2);
    secp256k1_fe_mul(&a->x, &a->x, &z2);
    secp256k1_fe_mul(&a->y, &a->y, &z3);
    secp256k1_fe_set_int(&a->z, 1);
    r->x = a->x;
    r->y = a->y;
}
static void secp256k1_ge_set_gej_var(secp256k1_ge *r, secp256k1_gej *a) {
    secp256k1_fe z2, z3;
    if (a->infinity) {
        secp256k1_ge_set_infinity(r);
        return;
    }
    secp256k1_fe_inv_var(&a->z, &a->z);
    secp256k1_fe_sqr(&z2, &a->z);
    secp256k1_fe_mul(&z3, &a->z, &z2);
    secp256k1_fe_mul(&a->x, &a->x, &z2);
    secp256k1_fe_mul(&a->y, &a->y, &z3);
    secp256k1_fe_set_int(&a->z, 1);
    secp256k1_ge_set_xy(r, &a->x, &a->y);
}
static void secp256k1_ge_set_all_gej_var(secp256k1_ge *r, const secp256k1_gej *a, size_t len) {
    secp256k1_fe u;
    size_t i;
    size_t last_i = SIZE_MAX;
    for (i = 0; i < len; i++) {
        if (a[i].infinity) {
            secp256k1_ge_set_infinity(&r[i]);
        } else {
            if (last_i == SIZE_MAX) {
                r[i].x = a[i].z;
            } else {
                secp256k1_fe_mul(&r[i].x, &r[last_i].x, &a[i].z);
            }
            last_i = i;
        }
    }
    if (last_i == SIZE_MAX) {
        return;
    }
    secp256k1_fe_inv_var(&u, &r[last_i].x);
    i = last_i;
    while (i > 0) {
        i--;
        if (!a[i].infinity) {
            secp256k1_fe_mul(&r[last_i].x, &r[i].x, &u);
            secp256k1_fe_mul(&u, &u, &a[last_i].z);
            last_i = i;
        }
    }
    VERIFY_CHECK(!a[last_i].infinity);
    r[last_i].x = u;
    for (i = 0; i < len; i++) {
        if (!a[i].infinity) {
            secp256k1_ge_set_gej_zinv(&r[i], &a[i], &r[i].x);
        }
    }
}
static void secp256k1_ge_table_set_globalz(size_t len, secp256k1_ge *a, const secp256k1_fe *zr) {
    size_t i = len - 1;
    secp256k1_fe zs;
    if (len > 0) {
        secp256k1_fe_normalize_weak(&a[i].y);
        zs = zr[i];
        while (i > 0) {
            secp256k1_gej tmpa;
            if (i != len - 1) {
                secp256k1_fe_mul(&zs, &zs, &zr[i]);
            }
            i--;
            tmpa.x = a[i].x;
            tmpa.y = a[i].y;
            tmpa.infinity = 0;
            secp256k1_ge_set_gej_zinv(&a[i], &tmpa, &zs);
        }
    }
}
static void secp256k1_gej_set_infinity(secp256k1_gej *r) {
    r->infinity = 1;
    secp256k1_fe_clear(&r->x);
    secp256k1_fe_clear(&r->y);
    secp256k1_fe_clear(&r->z);
}
static void secp256k1_ge_set_infinity(secp256k1_ge *r) {
    r->infinity = 1;
    secp256k1_fe_clear(&r->x);
    secp256k1_fe_clear(&r->y);
}
static void secp256k1_gej_clear(secp256k1_gej *r) {
    r->infinity = 0;
    secp256k1_fe_clear(&r->x);
    secp256k1_fe_clear(&r->y);
    secp256k1_fe_clear(&r->z);
}
static void secp256k1_ge_clear(secp256k1_ge *r) {
    r->infinity = 0;
    secp256k1_fe_clear(&r->x);
    secp256k1_fe_clear(&r->y);
}
static int secp256k1_ge_set_xo_var(secp256k1_ge *r, const secp256k1_fe *x, int odd) {
    secp256k1_fe x2, x3;
    r->x = *x;
    secp256k1_fe_sqr(&x2, x);
    secp256k1_fe_mul(&x3, x, &x2);
    r->infinity = 0;
    secp256k1_fe_add(&x3, &secp256k1_fe_const_b);
    if (!secp256k1_fe_sqrt(&r->y, &x3)) {
        return 0;
    }
    secp256k1_fe_normalize_var(&r->y);
    if (secp256k1_fe_is_odd(&r->y) != odd) {
        secp256k1_fe_negate(&r->y, &r->y, 1);
    }
    return 1;
}
static void secp256k1_gej_set_ge(secp256k1_gej *r, const secp256k1_ge *a) {
   r->infinity = a->infinity;
   r->x = a->x;
   r->y = a->y;
   secp256k1_fe_set_int(&r->z, 1);
}
static int secp256k1_gej_eq_x_var(const secp256k1_fe *x, const secp256k1_gej *a) {
    secp256k1_fe r, r2;
    VERIFY_CHECK(!a->infinity);
    secp256k1_fe_sqr(&r, &a->z); secp256k1_fe_mul(&r, &r, x);
    r2 = a->x; secp256k1_fe_normalize_weak(&r2);
    return secp256k1_fe_equal_var(&r, &r2);
}
static void secp256k1_gej_neg(secp256k1_gej *r, const secp256k1_gej *a) {
    r->infinity = a->infinity;
    r->x = a->x;
    r->y = a->y;
    r->z = a->z;
    secp256k1_fe_normalize_weak(&r->y);
    secp256k1_fe_negate(&r->y, &r->y, 1);
}
static int secp256k1_gej_is_infinity(const secp256k1_gej *a) {
    return a->infinity;
}
static int secp256k1_ge_is_valid_var(const secp256k1_ge *a) {
    secp256k1_fe y2, x3;
    if (a->infinity) {
        return 0;
    }
    secp256k1_fe_sqr(&y2, &a->y);
    secp256k1_fe_sqr(&x3, &a->x); secp256k1_fe_mul(&x3, &x3, &a->x);
    secp256k1_fe_add(&x3, &secp256k1_fe_const_b);
    secp256k1_fe_normalize_weak(&x3);
    return secp256k1_fe_equal_var(&y2, &x3);
}
static SECP256K1_INLINE void secp256k1_gej_double(secp256k1_gej *r, const secp256k1_gej *a) {
    secp256k1_fe l, s, t;
    r->infinity = a->infinity;
    secp256k1_fe_mul(&r->z, &a->z, &a->y);
    secp256k1_fe_sqr(&s, &a->y);
    secp256k1_fe_sqr(&l, &a->x);
    secp256k1_fe_mul_int(&l, 3);
    secp256k1_fe_half(&l);
    secp256k1_fe_negate(&t, &s, 1);
    secp256k1_fe_mul(&t, &t, &a->x);
    secp256k1_fe_sqr(&r->x, &l);
    secp256k1_fe_add(&r->x, &t);
    secp256k1_fe_add(&r->x, &t);
    secp256k1_fe_sqr(&s, &s);
    secp256k1_fe_add(&t, &r->x);
    secp256k1_fe_mul(&r->y, &t, &l);
    secp256k1_fe_add(&r->y, &s);
    secp256k1_fe_negate(&r->y, &r->y, 2);
}
static void secp256k1_gej_double_var(secp256k1_gej *r, const secp256k1_gej *a, secp256k1_fe *rzr) {
    if (a->infinity) {
        secp256k1_gej_set_infinity(r);
        if (rzr != NULL) {
            secp256k1_fe_set_int(rzr, 1);
        }
        return;
    }
    if (rzr != NULL) {
        *rzr = a->y;
        secp256k1_fe_normalize_weak(rzr);
    }
    secp256k1_gej_double(r, a);
}
static void secp256k1_gej_add_var(secp256k1_gej *r, const secp256k1_gej *a, const secp256k1_gej *b, secp256k1_fe *rzr) {
    secp256k1_fe z22, z12, u1, u2, s1, s2, h, i, h2, h3, t;
    if (a->infinity) {
        VERIFY_CHECK(rzr == NULL);
        *r = *b;
        return;
    }
    if (b->infinity) {
        if (rzr != NULL) {
            secp256k1_fe_set_int(rzr, 1);
        }
        *r = *a;
        return;
    }
    secp256k1_fe_sqr(&z22, &b->z);
    secp256k1_fe_sqr(&z12, &a->z);
    secp256k1_fe_mul(&u1, &a->x, &z22);
    secp256k1_fe_mul(&u2, &b->x, &z12);
    secp256k1_fe_mul(&s1, &a->y, &z22); secp256k1_fe_mul(&s1, &s1, &b->z);
    secp256k1_fe_mul(&s2, &b->y, &z12); secp256k1_fe_mul(&s2, &s2, &a->z);
    secp256k1_fe_negate(&h, &u1, 1); secp256k1_fe_add(&h, &u2);
    secp256k1_fe_negate(&i, &s2, 1); secp256k1_fe_add(&i, &s1);
    if (secp256k1_fe_normalizes_to_zero_var(&h)) {
        if (secp256k1_fe_normalizes_to_zero_var(&i)) {
            secp256k1_gej_double_var(r, a, rzr);
        } else {
            if (rzr != NULL) {
                secp256k1_fe_set_int(rzr, 0);
            }
            secp256k1_gej_set_infinity(r);
        }
        return;
    }
    r->infinity = 0;
    secp256k1_fe_mul(&t, &h, &b->z);
    if (rzr != NULL) {
        *rzr = t;
    }
    secp256k1_fe_mul(&r->z, &a->z, &t);
    secp256k1_fe_sqr(&h2, &h);
    secp256k1_fe_negate(&h2, &h2, 1);
    secp256k1_fe_mul(&h3, &h2, &h);
    secp256k1_fe_mul(&t, &u1, &h2);
    secp256k1_fe_sqr(&r->x, &i);
    secp256k1_fe_add(&r->x, &h3);
    secp256k1_fe_add(&r->x, &t);
    secp256k1_fe_add(&r->x, &t);
    secp256k1_fe_add(&t, &r->x);
    secp256k1_fe_mul(&r->y, &t, &i);
    secp256k1_fe_mul(&h3, &h3, &s1);
    secp256k1_fe_add(&r->y, &h3);
}
static void secp256k1_gej_add_ge_var(secp256k1_gej *r, const secp256k1_gej *a, const secp256k1_ge *b, secp256k1_fe *rzr) {
    secp256k1_fe z12, u1, u2, s1, s2, h, i, h2, h3, t;
    if (a->infinity) {
        VERIFY_CHECK(rzr == NULL);
        secp256k1_gej_set_ge(r, b);
        return;
    }
    if (b->infinity) {
        if (rzr != NULL) {
            secp256k1_fe_set_int(rzr, 1);
        }
        *r = *a;
        return;
    }
    secp256k1_fe_sqr(&z12, &a->z);
    u1 = a->x; secp256k1_fe_normalize_weak(&u1);
    secp256k1_fe_mul(&u2, &b->x, &z12);
    s1 = a->y; secp256k1_fe_normalize_weak(&s1);
    secp256k1_fe_mul(&s2, &b->y, &z12); secp256k1_fe_mul(&s2, &s2, &a->z);
    secp256k1_fe_negate(&h, &u1, 1); secp256k1_fe_add(&h, &u2);
    secp256k1_fe_negate(&i, &s2, 1); secp256k1_fe_add(&i, &s1);
    if (secp256k1_fe_normalizes_to_zero_var(&h)) {
        if (secp256k1_fe_normalizes_to_zero_var(&i)) {
            secp256k1_gej_double_var(r, a, rzr);
        } else {
            if (rzr != NULL) {
                secp256k1_fe_set_int(rzr, 0);
            }
            secp256k1_gej_set_infinity(r);
        }
        return;
    }
    r->infinity = 0;
    if (rzr != NULL) {
        *rzr = h;
    }
    secp256k1_fe_mul(&r->z, &a->z, &h);
    secp256k1_fe_sqr(&h2, &h);
    secp256k1_fe_negate(&h2, &h2, 1);
    secp256k1_fe_mul(&h3, &h2, &h);
    secp256k1_fe_mul(&t, &u1, &h2);
    secp256k1_fe_sqr(&r->x, &i);
    secp256k1_fe_add(&r->x, &h3);
    secp256k1_fe_add(&r->x, &t);
    secp256k1_fe_add(&r->x, &t);
    secp256k1_fe_add(&t, &r->x);
    secp256k1_fe_mul(&r->y, &t, &i);
    secp256k1_fe_mul(&h3, &h3, &s1);
    secp256k1_fe_add(&r->y, &h3);
}
static void secp256k1_gej_add_zinv_var(secp256k1_gej *r, const secp256k1_gej *a, const secp256k1_ge *b, const secp256k1_fe *bzinv) {
    secp256k1_fe az, z12, u1, u2, s1, s2, h, i, h2, h3, t;
    if (a->infinity) {
        secp256k1_fe bzinv2, bzinv3;
        r->infinity = b->infinity;
        secp256k1_fe_sqr(&bzinv2, bzinv);
        secp256k1_fe_mul(&bzinv3, &bzinv2, bzinv);
        secp256k1_fe_mul(&r->x, &b->x, &bzinv2);
        secp256k1_fe_mul(&r->y, &b->y, &bzinv3);
        secp256k1_fe_set_int(&r->z, 1);
        return;
    }
    if (b->infinity) {
        *r = *a;
        return;
    }
    secp256k1_fe_mul(&az, &a->z, bzinv);
    secp256k1_fe_sqr(&z12, &az);
    u1 = a->x; secp256k1_fe_normalize_weak(&u1);
    secp256k1_fe_mul(&u2, &b->x, &z12);
    s1 = a->y; secp256k1_fe_normalize_weak(&s1);
    secp256k1_fe_mul(&s2, &b->y, &z12); secp256k1_fe_mul(&s2, &s2, &az);
    secp256k1_fe_negate(&h, &u1, 1); secp256k1_fe_add(&h, &u2);
    secp256k1_fe_negate(&i, &s2, 1); secp256k1_fe_add(&i, &s1);
    if (secp256k1_fe_normalizes_to_zero_var(&h)) {
        if (secp256k1_fe_normalizes_to_zero_var(&i)) {
            secp256k1_gej_double_var(r, a, NULL);
        } else {
            secp256k1_gej_set_infinity(r);
        }
        return;
    }
    r->infinity = 0;
    secp256k1_fe_mul(&r->z, &a->z, &h);
    secp256k1_fe_sqr(&h2, &h);
    secp256k1_fe_negate(&h2, &h2, 1);
    secp256k1_fe_mul(&h3, &h2, &h);
    secp256k1_fe_mul(&t, &u1, &h2);
    secp256k1_fe_sqr(&r->x, &i);
    secp256k1_fe_add(&r->x, &h3);
    secp256k1_fe_add(&r->x, &t);
    secp256k1_fe_add(&r->x, &t);
    secp256k1_fe_add(&t, &r->x);
    secp256k1_fe_mul(&r->y, &t, &i);
    secp256k1_fe_mul(&h3, &h3, &s1);
    secp256k1_fe_add(&r->y, &h3);
}
static void secp256k1_gej_add_ge(secp256k1_gej *r, const secp256k1_gej *a, const secp256k1_ge *b) {
    secp256k1_fe zz, u1, u2, s1, s2, t, tt, m, n, q, rr;
    secp256k1_fe m_alt, rr_alt;
    int infinity, degenerate;
    VERIFY_CHECK(!b->infinity);
    VERIFY_CHECK(a->infinity == 0 || a->infinity == 1);
    secp256k1_fe_sqr(&zz, &a->z);
    u1 = a->x; secp256k1_fe_normalize_weak(&u1);
    secp256k1_fe_mul(&u2, &b->x, &zz);
    s1 = a->y; secp256k1_fe_normalize_weak(&s1);
    secp256k1_fe_mul(&s2, &b->y, &zz);
    secp256k1_fe_mul(&s2, &s2, &a->z);
    t = u1; secp256k1_fe_add(&t, &u2);
    m = s1; secp256k1_fe_add(&m, &s2);
    secp256k1_fe_sqr(&rr, &t);
    secp256k1_fe_negate(&m_alt, &u2, 1);
    secp256k1_fe_mul(&tt, &u1, &m_alt);
    secp256k1_fe_add(&rr, &tt);
    degenerate = secp256k1_fe_normalizes_to_zero(&m) &
                 secp256k1_fe_normalizes_to_zero(&rr);
    rr_alt = s1;
    secp256k1_fe_mul_int(&rr_alt, 2);
    secp256k1_fe_add(&m_alt, &u1);
    secp256k1_fe_cmov(&rr_alt, &rr, !degenerate);
    secp256k1_fe_cmov(&m_alt, &m, !degenerate);
    secp256k1_fe_sqr(&n, &m_alt);
    secp256k1_fe_negate(&q, &t, 2);
    secp256k1_fe_mul(&q, &q, &n);
    secp256k1_fe_sqr(&n, &n);
    secp256k1_fe_cmov(&n, &m, degenerate);
    secp256k1_fe_sqr(&t, &rr_alt);
    secp256k1_fe_mul(&r->z, &a->z, &m_alt);
    infinity = secp256k1_fe_normalizes_to_zero(&r->z) & ~a->infinity;
    secp256k1_fe_add(&t, &q);
    r->x = t;
    secp256k1_fe_mul_int(&t, 2);
    secp256k1_fe_add(&t, &q);
    secp256k1_fe_mul(&t, &t, &rr_alt);
    secp256k1_fe_add(&t, &n);
    secp256k1_fe_negate(&r->y, &t, 3);
    secp256k1_fe_half(&r->y);
    secp256k1_fe_cmov(&r->x, &b->x, a->infinity);
    secp256k1_fe_cmov(&r->y, &b->y, a->infinity);
    secp256k1_fe_cmov(&r->z, &secp256k1_fe_one, a->infinity);
    r->infinity = infinity;
}
static void secp256k1_gej_rescale(secp256k1_gej *r, const secp256k1_fe *s) {
    secp256k1_fe zz;
    VERIFY_CHECK(!secp256k1_fe_is_zero(s));
    secp256k1_fe_sqr(&zz, s);
    secp256k1_fe_mul(&r->x, &r->x, &zz);
    secp256k1_fe_mul(&r->y, &r->y, &zz);
    secp256k1_fe_mul(&r->y, &r->y, s);
    secp256k1_fe_mul(&r->z, &r->z, s);
}
static void secp256k1_ge_to_storage(secp256k1_ge_storage *r, const secp256k1_ge *a) {
    secp256k1_fe x, y;
    VERIFY_CHECK(!a->infinity);
    x = a->x;
    secp256k1_fe_normalize(&x);
    y = a->y;
    secp256k1_fe_normalize(&y);
    secp256k1_fe_to_storage(&r->x, &x);
    secp256k1_fe_to_storage(&r->y, &y);
}
static void secp256k1_ge_from_storage(secp256k1_ge *r, const secp256k1_ge_storage *a) {
    secp256k1_fe_from_storage(&r->x, &a->x);
    secp256k1_fe_from_storage(&r->y, &a->y);
    r->infinity = 0;
}
static SECP256K1_INLINE void secp256k1_gej_cmov(secp256k1_gej *r, const secp256k1_gej *a, int flag) {
    secp256k1_fe_cmov(&r->x, &a->x, flag);
    secp256k1_fe_cmov(&r->y, &a->y, flag);
    secp256k1_fe_cmov(&r->z, &a->z, flag);
    r->infinity ^= (r->infinity ^ a->infinity) & flag;
}
static SECP256K1_INLINE void secp256k1_ge_storage_cmov(secp256k1_ge_storage *r, const secp256k1_ge_storage *a, int flag) {
    secp256k1_fe_storage_cmov(&r->x, &a->x, flag);
    secp256k1_fe_storage_cmov(&r->y, &a->y, flag);
}
static void secp256k1_ge_mul_lambda(secp256k1_ge *r, const secp256k1_ge *a) {
    *r = *a;
    secp256k1_fe_mul(&r->x, &r->x, &secp256k1_const_beta);
}
static int secp256k1_ge_is_in_correct_subgroup(const secp256k1_ge* ge) {
#ifdef EXHAUSTIVE_TEST_ORDER
    secp256k1_gej out;
    int i;
    secp256k1_gej_set_infinity(&out);
    for (i = 0; i < 32; ++i) {
        secp256k1_gej_double_var(&out, &out, NULL);
        if ((((uint32_t)EXHAUSTIVE_TEST_ORDER) >> (31 - i)) & 1) {
            secp256k1_gej_add_ge_var(&out, &out, ge, NULL);
        }
    }
    return secp256k1_gej_is_infinity(&out);
#else
    (void)ge;
    return 1;
#endif
}
#endif
