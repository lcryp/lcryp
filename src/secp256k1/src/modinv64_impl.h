#ifndef SECP256K1_MODINV64_IMPL_H
#define SECP256K1_MODINV64_IMPL_H
#include "modinv64.h"
#include "util.h"
#ifdef VERIFY
static int64_t secp256k1_modinv64_abs(int64_t v) {
    VERIFY_CHECK(v > INT64_MIN);
    if (v < 0) return -v;
    return v;
}
static const secp256k1_modinv64_signed62 SECP256K1_SIGNED62_ONE = {{1}};
static void secp256k1_modinv64_mul_62(secp256k1_modinv64_signed62 *r, const secp256k1_modinv64_signed62 *a, int alen, int64_t factor) {
    const int64_t M62 = (int64_t)(UINT64_MAX >> 2);
    int128_t c = 0;
    int i;
    for (i = 0; i < 4; ++i) {
        if (i < alen) c += (int128_t)a->v[i] * factor;
        r->v[i] = (int64_t)c & M62; c >>= 62;
    }
    if (4 < alen) c += (int128_t)a->v[4] * factor;
    VERIFY_CHECK(c == (int64_t)c);
    r->v[4] = (int64_t)c;
}
static int secp256k1_modinv64_mul_cmp_62(const secp256k1_modinv64_signed62 *a, int alen, const secp256k1_modinv64_signed62 *b, int64_t factor) {
    int i;
    secp256k1_modinv64_signed62 am, bm;
    secp256k1_modinv64_mul_62(&am, a, alen, 1);
    secp256k1_modinv64_mul_62(&bm, b, 5, factor);
    for (i = 0; i < 4; ++i) {
        VERIFY_CHECK(am.v[i] >> 62 == 0);
        VERIFY_CHECK(bm.v[i] >> 62 == 0);
    }
    for (i = 4; i >= 0; --i) {
        if (am.v[i] < bm.v[i]) return -1;
        if (am.v[i] > bm.v[i]) return 1;
    }
    return 0;
}
#endif
static void secp256k1_modinv64_normalize_62(secp256k1_modinv64_signed62 *r, int64_t sign, const secp256k1_modinv64_modinfo *modinfo) {
    const int64_t M62 = (int64_t)(UINT64_MAX >> 2);
    int64_t r0 = r->v[0], r1 = r->v[1], r2 = r->v[2], r3 = r->v[3], r4 = r->v[4];
    int64_t cond_add, cond_negate;
#ifdef VERIFY
    int i;
    for (i = 0; i < 5; ++i) {
        VERIFY_CHECK(r->v[i] >= -M62);
        VERIFY_CHECK(r->v[i] <= M62);
    }
    VERIFY_CHECK(secp256k1_modinv64_mul_cmp_62(r, 5, &modinfo->modulus, -2) > 0);
    VERIFY_CHECK(secp256k1_modinv64_mul_cmp_62(r, 5, &modinfo->modulus, 1) < 0);
#endif
    cond_add = r4 >> 63;
    r0 += modinfo->modulus.v[0] & cond_add;
    r1 += modinfo->modulus.v[1] & cond_add;
    r2 += modinfo->modulus.v[2] & cond_add;
    r3 += modinfo->modulus.v[3] & cond_add;
    r4 += modinfo->modulus.v[4] & cond_add;
    cond_negate = sign >> 63;
    r0 = (r0 ^ cond_negate) - cond_negate;
    r1 = (r1 ^ cond_negate) - cond_negate;
    r2 = (r2 ^ cond_negate) - cond_negate;
    r3 = (r3 ^ cond_negate) - cond_negate;
    r4 = (r4 ^ cond_negate) - cond_negate;
    r1 += r0 >> 62; r0 &= M62;
    r2 += r1 >> 62; r1 &= M62;
    r3 += r2 >> 62; r2 &= M62;
    r4 += r3 >> 62; r3 &= M62;
    cond_add = r4 >> 63;
    r0 += modinfo->modulus.v[0] & cond_add;
    r1 += modinfo->modulus.v[1] & cond_add;
    r2 += modinfo->modulus.v[2] & cond_add;
    r3 += modinfo->modulus.v[3] & cond_add;
    r4 += modinfo->modulus.v[4] & cond_add;
    r1 += r0 >> 62; r0 &= M62;
    r2 += r1 >> 62; r1 &= M62;
    r3 += r2 >> 62; r2 &= M62;
    r4 += r3 >> 62; r3 &= M62;
    r->v[0] = r0;
    r->v[1] = r1;
    r->v[2] = r2;
    r->v[3] = r3;
    r->v[4] = r4;
#ifdef VERIFY
    VERIFY_CHECK(r0 >> 62 == 0);
    VERIFY_CHECK(r1 >> 62 == 0);
    VERIFY_CHECK(r2 >> 62 == 0);
    VERIFY_CHECK(r3 >> 62 == 0);
    VERIFY_CHECK(r4 >> 62 == 0);
    VERIFY_CHECK(secp256k1_modinv64_mul_cmp_62(r, 5, &modinfo->modulus, 0) >= 0);
    VERIFY_CHECK(secp256k1_modinv64_mul_cmp_62(r, 5, &modinfo->modulus, 1) < 0);
#endif
}
typedef struct {
    int64_t u, v, q, r;
} secp256k1_modinv64_trans2x2;
static int64_t secp256k1_modinv64_divsteps_59(int64_t zeta, uint64_t f0, uint64_t g0, secp256k1_modinv64_trans2x2 *t) {
    uint64_t u = 8, v = 0, q = 0, r = 8;
    uint64_t c1, c2, f = f0, g = g0, x, y, z;
    int i;
    for (i = 3; i < 62; ++i) {
        VERIFY_CHECK((f & 1) == 1);
        VERIFY_CHECK((u * f0 + v * g0) == f << i);
        VERIFY_CHECK((q * f0 + r * g0) == g << i);
        c1 = zeta >> 63;
        c2 = -(g & 1);
        x = (f ^ c1) - c1;
        y = (u ^ c1) - c1;
        z = (v ^ c1) - c1;
        g += x & c2;
        q += y & c2;
        r += z & c2;
        c1 &= c2;
        zeta = (zeta ^ c1) - 1;
        f += g & c1;
        u += q & c1;
        v += r & c1;
        g >>= 1;
        u <<= 1;
        v <<= 1;
        VERIFY_CHECK(zeta >= -591 && zeta <= 591);
    }
    t->u = (int64_t)u;
    t->v = (int64_t)v;
    t->q = (int64_t)q;
    t->r = (int64_t)r;
    VERIFY_CHECK((int128_t)t->u * t->r - (int128_t)t->v * t->q == ((int128_t)1) << 65);
    return zeta;
}
static int64_t secp256k1_modinv64_divsteps_62_var(int64_t eta, uint64_t f0, uint64_t g0, secp256k1_modinv64_trans2x2 *t) {
    uint64_t u = 1, v = 0, q = 0, r = 1;
    uint64_t f = f0, g = g0, m;
    uint32_t w;
    int i = 62, limit, zeros;
    for (;;) {
        zeros = secp256k1_ctz64_var(g | (UINT64_MAX << i));
        g >>= zeros;
        u <<= zeros;
        v <<= zeros;
        eta -= zeros;
        i -= zeros;
        if (i == 0) break;
        VERIFY_CHECK((f & 1) == 1);
        VERIFY_CHECK((g & 1) == 1);
        VERIFY_CHECK((u * f0 + v * g0) == f << (62 - i));
        VERIFY_CHECK((q * f0 + r * g0) == g << (62 - i));
        VERIFY_CHECK(eta >= -745 && eta <= 745);
        if (eta < 0) {
            uint64_t tmp;
            eta = -eta;
            tmp = f; f = g; g = -tmp;
            tmp = u; u = q; q = -tmp;
            tmp = v; v = r; r = -tmp;
            limit = ((int)eta + 1) > i ? i : ((int)eta + 1);
            VERIFY_CHECK(limit > 0 && limit <= 62);
            m = (UINT64_MAX >> (64 - limit)) & 63U;
            w = (f * g * (f * f - 2)) & m;
        } else {
            limit = ((int)eta + 1) > i ? i : ((int)eta + 1);
            VERIFY_CHECK(limit > 0 && limit <= 62);
            m = (UINT64_MAX >> (64 - limit)) & 15U;
            w = f + (((f + 1) & 4) << 1);
            w = (-w * g) & m;
        }
        g += f * w;
        q += u * w;
        r += v * w;
        VERIFY_CHECK((g & m) == 0);
    }
    t->u = (int64_t)u;
    t->v = (int64_t)v;
    t->q = (int64_t)q;
    t->r = (int64_t)r;
    VERIFY_CHECK((int128_t)t->u * t->r - (int128_t)t->v * t->q == ((int128_t)1) << 62);
    return eta;
}
static void secp256k1_modinv64_update_de_62(secp256k1_modinv64_signed62 *d, secp256k1_modinv64_signed62 *e, const secp256k1_modinv64_trans2x2 *t, const secp256k1_modinv64_modinfo* modinfo) {
    const int64_t M62 = (int64_t)(UINT64_MAX >> 2);
    const int64_t d0 = d->v[0], d1 = d->v[1], d2 = d->v[2], d3 = d->v[3], d4 = d->v[4];
    const int64_t e0 = e->v[0], e1 = e->v[1], e2 = e->v[2], e3 = e->v[3], e4 = e->v[4];
    const int64_t u = t->u, v = t->v, q = t->q, r = t->r;
    int64_t md, me, sd, se;
    int128_t cd, ce;
#ifdef VERIFY
    VERIFY_CHECK(secp256k1_modinv64_mul_cmp_62(d, 5, &modinfo->modulus, -2) > 0);
    VERIFY_CHECK(secp256k1_modinv64_mul_cmp_62(d, 5, &modinfo->modulus, 1) < 0);
    VERIFY_CHECK(secp256k1_modinv64_mul_cmp_62(e, 5, &modinfo->modulus, -2) > 0);
    VERIFY_CHECK(secp256k1_modinv64_mul_cmp_62(e, 5, &modinfo->modulus, 1) < 0);
    VERIFY_CHECK((secp256k1_modinv64_abs(u) + secp256k1_modinv64_abs(v)) >= 0);
    VERIFY_CHECK((secp256k1_modinv64_abs(q) + secp256k1_modinv64_abs(r)) >= 0);
    VERIFY_CHECK((secp256k1_modinv64_abs(u) + secp256k1_modinv64_abs(v)) <= M62 + 1);
    VERIFY_CHECK((secp256k1_modinv64_abs(q) + secp256k1_modinv64_abs(r)) <= M62 + 1);
#endif
    sd = d4 >> 63;
    se = e4 >> 63;
    md = (u & sd) + (v & se);
    me = (q & sd) + (r & se);
    cd = (int128_t)u * d0 + (int128_t)v * e0;
    ce = (int128_t)q * d0 + (int128_t)r * e0;
    md -= (modinfo->modulus_inv62 * (uint64_t)cd + md) & M62;
    me -= (modinfo->modulus_inv62 * (uint64_t)ce + me) & M62;
    cd += (int128_t)modinfo->modulus.v[0] * md;
    ce += (int128_t)modinfo->modulus.v[0] * me;
    VERIFY_CHECK(((int64_t)cd & M62) == 0); cd >>= 62;
    VERIFY_CHECK(((int64_t)ce & M62) == 0); ce >>= 62;
    cd += (int128_t)u * d1 + (int128_t)v * e1;
    ce += (int128_t)q * d1 + (int128_t)r * e1;
    if (modinfo->modulus.v[1]) {
        cd += (int128_t)modinfo->modulus.v[1] * md;
        ce += (int128_t)modinfo->modulus.v[1] * me;
    }
    d->v[0] = (int64_t)cd & M62; cd >>= 62;
    e->v[0] = (int64_t)ce & M62; ce >>= 62;
    cd += (int128_t)u * d2 + (int128_t)v * e2;
    ce += (int128_t)q * d2 + (int128_t)r * e2;
    if (modinfo->modulus.v[2]) {
        cd += (int128_t)modinfo->modulus.v[2] * md;
        ce += (int128_t)modinfo->modulus.v[2] * me;
    }
    d->v[1] = (int64_t)cd & M62; cd >>= 62;
    e->v[1] = (int64_t)ce & M62; ce >>= 62;
    cd += (int128_t)u * d3 + (int128_t)v * e3;
    ce += (int128_t)q * d3 + (int128_t)r * e3;
    if (modinfo->modulus.v[3]) {
        cd += (int128_t)modinfo->modulus.v[3] * md;
        ce += (int128_t)modinfo->modulus.v[3] * me;
    }
    d->v[2] = (int64_t)cd & M62; cd >>= 62;
    e->v[2] = (int64_t)ce & M62; ce >>= 62;
    cd += (int128_t)u * d4 + (int128_t)v * e4;
    ce += (int128_t)q * d4 + (int128_t)r * e4;
    cd += (int128_t)modinfo->modulus.v[4] * md;
    ce += (int128_t)modinfo->modulus.v[4] * me;
    d->v[3] = (int64_t)cd & M62; cd >>= 62;
    e->v[3] = (int64_t)ce & M62; ce >>= 62;
    d->v[4] = (int64_t)cd;
    e->v[4] = (int64_t)ce;
#ifdef VERIFY
    VERIFY_CHECK(secp256k1_modinv64_mul_cmp_62(d, 5, &modinfo->modulus, -2) > 0);
    VERIFY_CHECK(secp256k1_modinv64_mul_cmp_62(d, 5, &modinfo->modulus, 1) < 0);
    VERIFY_CHECK(secp256k1_modinv64_mul_cmp_62(e, 5, &modinfo->modulus, -2) > 0);
    VERIFY_CHECK(secp256k1_modinv64_mul_cmp_62(e, 5, &modinfo->modulus, 1) < 0);
#endif
}
static void secp256k1_modinv64_update_fg_62(secp256k1_modinv64_signed62 *f, secp256k1_modinv64_signed62 *g, const secp256k1_modinv64_trans2x2 *t) {
    const int64_t M62 = (int64_t)(UINT64_MAX >> 2);
    const int64_t f0 = f->v[0], f1 = f->v[1], f2 = f->v[2], f3 = f->v[3], f4 = f->v[4];
    const int64_t g0 = g->v[0], g1 = g->v[1], g2 = g->v[2], g3 = g->v[3], g4 = g->v[4];
    const int64_t u = t->u, v = t->v, q = t->q, r = t->r;
    int128_t cf, cg;
    cf = (int128_t)u * f0 + (int128_t)v * g0;
    cg = (int128_t)q * f0 + (int128_t)r * g0;
    VERIFY_CHECK(((int64_t)cf & M62) == 0); cf >>= 62;
    VERIFY_CHECK(((int64_t)cg & M62) == 0); cg >>= 62;
    cf += (int128_t)u * f1 + (int128_t)v * g1;
    cg += (int128_t)q * f1 + (int128_t)r * g1;
    f->v[0] = (int64_t)cf & M62; cf >>= 62;
    g->v[0] = (int64_t)cg & M62; cg >>= 62;
    cf += (int128_t)u * f2 + (int128_t)v * g2;
    cg += (int128_t)q * f2 + (int128_t)r * g2;
    f->v[1] = (int64_t)cf & M62; cf >>= 62;
    g->v[1] = (int64_t)cg & M62; cg >>= 62;
    cf += (int128_t)u * f3 + (int128_t)v * g3;
    cg += (int128_t)q * f3 + (int128_t)r * g3;
    f->v[2] = (int64_t)cf & M62; cf >>= 62;
    g->v[2] = (int64_t)cg & M62; cg >>= 62;
    cf += (int128_t)u * f4 + (int128_t)v * g4;
    cg += (int128_t)q * f4 + (int128_t)r * g4;
    f->v[3] = (int64_t)cf & M62; cf >>= 62;
    g->v[3] = (int64_t)cg & M62; cg >>= 62;
    f->v[4] = (int64_t)cf;
    g->v[4] = (int64_t)cg;
}
static void secp256k1_modinv64_update_fg_62_var(int len, secp256k1_modinv64_signed62 *f, secp256k1_modinv64_signed62 *g, const secp256k1_modinv64_trans2x2 *t) {
    const int64_t M62 = (int64_t)(UINT64_MAX >> 2);
    const int64_t u = t->u, v = t->v, q = t->q, r = t->r;
    int64_t fi, gi;
    int128_t cf, cg;
    int i;
    VERIFY_CHECK(len > 0);
    fi = f->v[0];
    gi = g->v[0];
    cf = (int128_t)u * fi + (int128_t)v * gi;
    cg = (int128_t)q * fi + (int128_t)r * gi;
    VERIFY_CHECK(((int64_t)cf & M62) == 0); cf >>= 62;
    VERIFY_CHECK(((int64_t)cg & M62) == 0); cg >>= 62;
    for (i = 1; i < len; ++i) {
        fi = f->v[i];
        gi = g->v[i];
        cf += (int128_t)u * fi + (int128_t)v * gi;
        cg += (int128_t)q * fi + (int128_t)r * gi;
        f->v[i - 1] = (int64_t)cf & M62; cf >>= 62;
        g->v[i - 1] = (int64_t)cg & M62; cg >>= 62;
    }
    f->v[len - 1] = (int64_t)cf;
    g->v[len - 1] = (int64_t)cg;
}
static void secp256k1_modinv64(secp256k1_modinv64_signed62 *x, const secp256k1_modinv64_modinfo *modinfo) {
    secp256k1_modinv64_signed62 d = {{0, 0, 0, 0, 0}};
    secp256k1_modinv64_signed62 e = {{1, 0, 0, 0, 0}};
    secp256k1_modinv64_signed62 f = modinfo->modulus;
    secp256k1_modinv64_signed62 g = *x;
    int i;
    int64_t zeta = -1;
    for (i = 0; i < 10; ++i) {
        secp256k1_modinv64_trans2x2 t;
        zeta = secp256k1_modinv64_divsteps_59(zeta, f.v[0], g.v[0], &t);
        secp256k1_modinv64_update_de_62(&d, &e, &t, modinfo);
#ifdef VERIFY
        VERIFY_CHECK(secp256k1_modinv64_mul_cmp_62(&f, 5, &modinfo->modulus, -1) > 0);
        VERIFY_CHECK(secp256k1_modinv64_mul_cmp_62(&f, 5, &modinfo->modulus, 1) <= 0);
        VERIFY_CHECK(secp256k1_modinv64_mul_cmp_62(&g, 5, &modinfo->modulus, -1) > 0);
        VERIFY_CHECK(secp256k1_modinv64_mul_cmp_62(&g, 5, &modinfo->modulus, 1) < 0);
#endif
        secp256k1_modinv64_update_fg_62(&f, &g, &t);
#ifdef VERIFY
        VERIFY_CHECK(secp256k1_modinv64_mul_cmp_62(&f, 5, &modinfo->modulus, -1) > 0);
        VERIFY_CHECK(secp256k1_modinv64_mul_cmp_62(&f, 5, &modinfo->modulus, 1) <= 0);
        VERIFY_CHECK(secp256k1_modinv64_mul_cmp_62(&g, 5, &modinfo->modulus, -1) > 0);
        VERIFY_CHECK(secp256k1_modinv64_mul_cmp_62(&g, 5, &modinfo->modulus, 1) < 0);
#endif
    }
#ifdef VERIFY
    VERIFY_CHECK(secp256k1_modinv64_mul_cmp_62(&g, 5, &SECP256K1_SIGNED62_ONE, 0) == 0);
    VERIFY_CHECK(secp256k1_modinv64_mul_cmp_62(&f, 5, &SECP256K1_SIGNED62_ONE, -1) == 0 ||
                 secp256k1_modinv64_mul_cmp_62(&f, 5, &SECP256K1_SIGNED62_ONE, 1) == 0 ||
                 (secp256k1_modinv64_mul_cmp_62(x, 5, &SECP256K1_SIGNED62_ONE, 0) == 0 &&
                  secp256k1_modinv64_mul_cmp_62(&d, 5, &SECP256K1_SIGNED62_ONE, 0) == 0 &&
                  (secp256k1_modinv64_mul_cmp_62(&f, 5, &modinfo->modulus, 1) == 0 ||
                   secp256k1_modinv64_mul_cmp_62(&f, 5, &modinfo->modulus, -1) == 0)));
#endif
    secp256k1_modinv64_normalize_62(&d, f.v[4], modinfo);
    *x = d;
}
static void secp256k1_modinv64_var(secp256k1_modinv64_signed62 *x, const secp256k1_modinv64_modinfo *modinfo) {
    secp256k1_modinv64_signed62 d = {{0, 0, 0, 0, 0}};
    secp256k1_modinv64_signed62 e = {{1, 0, 0, 0, 0}};
    secp256k1_modinv64_signed62 f = modinfo->modulus;
    secp256k1_modinv64_signed62 g = *x;
#ifdef VERIFY
    int i = 0;
#endif
    int j, len = 5;
    int64_t eta = -1;
    int64_t cond, fn, gn;
    while (1) {
        secp256k1_modinv64_trans2x2 t;
        eta = secp256k1_modinv64_divsteps_62_var(eta, f.v[0], g.v[0], &t);
        secp256k1_modinv64_update_de_62(&d, &e, &t, modinfo);
#ifdef VERIFY
        VERIFY_CHECK(secp256k1_modinv64_mul_cmp_62(&f, len, &modinfo->modulus, -1) > 0);
        VERIFY_CHECK(secp256k1_modinv64_mul_cmp_62(&f, len, &modinfo->modulus, 1) <= 0);
        VERIFY_CHECK(secp256k1_modinv64_mul_cmp_62(&g, len, &modinfo->modulus, -1) > 0);
        VERIFY_CHECK(secp256k1_modinv64_mul_cmp_62(&g, len, &modinfo->modulus, 1) < 0);
#endif
        secp256k1_modinv64_update_fg_62_var(len, &f, &g, &t);
        if (g.v[0] == 0) {
            cond = 0;
            for (j = 1; j < len; ++j) {
                cond |= g.v[j];
            }
            if (cond == 0) break;
        }
        fn = f.v[len - 1];
        gn = g.v[len - 1];
        cond = ((int64_t)len - 2) >> 63;
        cond |= fn ^ (fn >> 63);
        cond |= gn ^ (gn >> 63);
        if (cond == 0) {
            f.v[len - 2] |= (uint64_t)fn << 62;
            g.v[len - 2] |= (uint64_t)gn << 62;
            --len;
        }
#ifdef VERIFY
        VERIFY_CHECK(++i < 12);
        VERIFY_CHECK(secp256k1_modinv64_mul_cmp_62(&f, len, &modinfo->modulus, -1) > 0);
        VERIFY_CHECK(secp256k1_modinv64_mul_cmp_62(&f, len, &modinfo->modulus, 1) <= 0);
        VERIFY_CHECK(secp256k1_modinv64_mul_cmp_62(&g, len, &modinfo->modulus, -1) > 0);
        VERIFY_CHECK(secp256k1_modinv64_mul_cmp_62(&g, len, &modinfo->modulus, 1) < 0);
#endif
    }
#ifdef VERIFY
    VERIFY_CHECK(secp256k1_modinv64_mul_cmp_62(&g, len, &SECP256K1_SIGNED62_ONE, 0) == 0);
    VERIFY_CHECK(secp256k1_modinv64_mul_cmp_62(&f, len, &SECP256K1_SIGNED62_ONE, -1) == 0 ||
                 secp256k1_modinv64_mul_cmp_62(&f, len, &SECP256K1_SIGNED62_ONE, 1) == 0 ||
                 (secp256k1_modinv64_mul_cmp_62(x, 5, &SECP256K1_SIGNED62_ONE, 0) == 0 &&
                  secp256k1_modinv64_mul_cmp_62(&d, 5, &SECP256K1_SIGNED62_ONE, 0) == 0 &&
                  (secp256k1_modinv64_mul_cmp_62(&f, len, &modinfo->modulus, 1) == 0 ||
                   secp256k1_modinv64_mul_cmp_62(&f, len, &modinfo->modulus, -1) == 0)));
#endif
    secp256k1_modinv64_normalize_62(&d, f.v[len - 1], modinfo);
    *x = d;
}
#endif
