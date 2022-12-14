#ifndef SECP256K1_MODINV32_H
#define SECP256K1_MODINV32_H
#if defined HAVE_CONFIG_H
#include "libsecp256k1-config.h"
#endif
#include "util.h"
typedef struct {
    int32_t v[9];
} secp256k1_modinv32_signed30;
typedef struct {
    secp256k1_modinv32_signed30 modulus;
    uint32_t modulus_inv30;
} secp256k1_modinv32_modinfo;
static void secp256k1_modinv32_var(secp256k1_modinv32_signed30 *x, const secp256k1_modinv32_modinfo *modinfo);
static void secp256k1_modinv32(secp256k1_modinv32_signed30 *x, const secp256k1_modinv32_modinfo *modinfo);
#endif
