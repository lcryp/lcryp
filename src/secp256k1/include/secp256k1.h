#ifndef SECP256K1_H
#define SECP256K1_H
#ifdef __cplusplus
extern "C" {
#endif
#include <stddef.h>
typedef struct secp256k1_context_struct secp256k1_context;
typedef struct secp256k1_scratch_space_struct secp256k1_scratch_space;
typedef struct {
    unsigned char data[64];
} secp256k1_pubkey;
typedef struct {
    unsigned char data[64];
} secp256k1_ecdsa_signature;
typedef int (*secp256k1_nonce_function)(
    unsigned char *nonce32,
    const unsigned char *msg32,
    const unsigned char *key32,
    const unsigned char *algo16,
    void *data,
    unsigned int attempt
);
# if !defined(SECP256K1_GNUC_PREREQ)
#  if defined(__GNUC__)&&defined(__GNUC_MINOR__)
#   define SECP256K1_GNUC_PREREQ(_maj,_min) \
 ((__GNUC__<<16)+__GNUC_MINOR__>=((_maj)<<16)+(_min))
#  else
#   define SECP256K1_GNUC_PREREQ(_maj,_min) 0
#  endif
# endif
# if (!defined(__STDC_VERSION__) || (__STDC_VERSION__ < 199901L) )
#  if SECP256K1_GNUC_PREREQ(2,7)
#   define SECP256K1_INLINE __inline__
#  elif (defined(_MSC_VER))
#   define SECP256K1_INLINE __inline
#  else
#   define SECP256K1_INLINE
#  endif
# else
#  define SECP256K1_INLINE inline
# endif
#ifndef SECP256K1_BUILD
# define SECP256K1_NO_BUILD
#endif
#ifndef SECP256K1_API
# if defined(_WIN32)
#  if defined(SECP256K1_BUILD) && defined(DLL_EXPORT)
#   define SECP256K1_API __declspec(dllexport)
#  else
#   define SECP256K1_API
#  endif
# elif defined(__GNUC__) && (__GNUC__ >= 4) && defined(SECP256K1_BUILD)
#  define SECP256K1_API __attribute__ ((visibility ("default")))
# else
#  define SECP256K1_API
# endif
#endif
# if defined(__GNUC__) && SECP256K1_GNUC_PREREQ(3, 4)
#  define SECP256K1_WARN_UNUSED_RESULT __attribute__ ((__warn_unused_result__))
# else
#  define SECP256K1_WARN_UNUSED_RESULT
# endif
# if !defined(SECP256K1_BUILD) && defined(__GNUC__) && SECP256K1_GNUC_PREREQ(3, 4)
#  define SECP256K1_ARG_NONNULL(_x)  __attribute__ ((__nonnull__(_x)))
# else
#  define SECP256K1_ARG_NONNULL(_x)
# endif
#if !defined(SECP256K1_BUILD) && defined(__has_attribute)
# if __has_attribute(__deprecated__)
#  define SECP256K1_DEPRECATED(_msg) __attribute__ ((__deprecated__(_msg)))
# else
#  define SECP256K1_DEPRECATED(_msg)
# endif
#else
# define SECP256K1_DEPRECATED(_msg)
#endif
#define SECP256K1_FLAGS_TYPE_MASK ((1 << 8) - 1)
#define SECP256K1_FLAGS_TYPE_CONTEXT (1 << 0)
#define SECP256K1_FLAGS_TYPE_COMPRESSION (1 << 1)
#define SECP256K1_FLAGS_BIT_CONTEXT_VERIFY (1 << 8)
#define SECP256K1_FLAGS_BIT_CONTEXT_SIGN (1 << 9)
#define SECP256K1_FLAGS_BIT_CONTEXT_DECLASSIFY (1 << 10)
#define SECP256K1_FLAGS_BIT_COMPRESSION (1 << 8)
#define SECP256K1_CONTEXT_VERIFY (SECP256K1_FLAGS_TYPE_CONTEXT | SECP256K1_FLAGS_BIT_CONTEXT_VERIFY)
#define SECP256K1_CONTEXT_SIGN (SECP256K1_FLAGS_TYPE_CONTEXT | SECP256K1_FLAGS_BIT_CONTEXT_SIGN)
#define SECP256K1_CONTEXT_DECLASSIFY (SECP256K1_FLAGS_TYPE_CONTEXT | SECP256K1_FLAGS_BIT_CONTEXT_DECLASSIFY)
#define SECP256K1_CONTEXT_NONE (SECP256K1_FLAGS_TYPE_CONTEXT)
#define SECP256K1_EC_COMPRESSED (SECP256K1_FLAGS_TYPE_COMPRESSION | SECP256K1_FLAGS_BIT_COMPRESSION)
#define SECP256K1_EC_UNCOMPRESSED (SECP256K1_FLAGS_TYPE_COMPRESSION)
#define SECP256K1_TAG_PUBKEY_EVEN 0x02
#define SECP256K1_TAG_PUBKEY_ODD 0x03
#define SECP256K1_TAG_PUBKEY_UNCOMPRESSED 0x04
#define SECP256K1_TAG_PUBKEY_HYBRID_EVEN 0x06
#define SECP256K1_TAG_PUBKEY_HYBRID_ODD 0x07
SECP256K1_API extern const secp256k1_context *secp256k1_context_no_precomp;
SECP256K1_API secp256k1_context* secp256k1_context_create(
    unsigned int flags
) SECP256K1_WARN_UNUSED_RESULT;
SECP256K1_API secp256k1_context* secp256k1_context_clone(
    const secp256k1_context* ctx
) SECP256K1_ARG_NONNULL(1) SECP256K1_WARN_UNUSED_RESULT;
SECP256K1_API void secp256k1_context_destroy(
    secp256k1_context* ctx
) SECP256K1_ARG_NONNULL(1);
SECP256K1_API void secp256k1_context_set_illegal_callback(
    secp256k1_context* ctx,
    void (*fun)(const char* message, void* data),
    const void* data
) SECP256K1_ARG_NONNULL(1);
SECP256K1_API void secp256k1_context_set_error_callback(
    secp256k1_context* ctx,
    void (*fun)(const char* message, void* data),
    const void* data
) SECP256K1_ARG_NONNULL(1);
SECP256K1_API SECP256K1_WARN_UNUSED_RESULT secp256k1_scratch_space* secp256k1_scratch_space_create(
    const secp256k1_context* ctx,
    size_t size
) SECP256K1_ARG_NONNULL(1);
SECP256K1_API void secp256k1_scratch_space_destroy(
    const secp256k1_context* ctx,
    secp256k1_scratch_space* scratch
) SECP256K1_ARG_NONNULL(1);
SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_ec_pubkey_parse(
    const secp256k1_context* ctx,
    secp256k1_pubkey* pubkey,
    const unsigned char *input,
    size_t inputlen
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3);
SECP256K1_API int secp256k1_ec_pubkey_serialize(
    const secp256k1_context* ctx,
    unsigned char *output,
    size_t *outputlen,
    const secp256k1_pubkey* pubkey,
    unsigned int flags
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3) SECP256K1_ARG_NONNULL(4);
SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_ec_pubkey_cmp(
    const secp256k1_context* ctx,
    const secp256k1_pubkey* pubkey1,
    const secp256k1_pubkey* pubkey2
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3);
SECP256K1_API int secp256k1_ecdsa_signature_parse_compact(
    const secp256k1_context* ctx,
    secp256k1_ecdsa_signature* sig,
    const unsigned char *input64
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3);
SECP256K1_API int secp256k1_ecdsa_signature_parse_der(
    const secp256k1_context* ctx,
    secp256k1_ecdsa_signature* sig,
    const unsigned char *input,
    size_t inputlen
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3);
SECP256K1_API int secp256k1_ecdsa_signature_serialize_der(
    const secp256k1_context* ctx,
    unsigned char *output,
    size_t *outputlen,
    const secp256k1_ecdsa_signature* sig
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3) SECP256K1_ARG_NONNULL(4);
SECP256K1_API int secp256k1_ecdsa_signature_serialize_compact(
    const secp256k1_context* ctx,
    unsigned char *output64,
    const secp256k1_ecdsa_signature* sig
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3);
SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_ecdsa_verify(
    const secp256k1_context* ctx,
    const secp256k1_ecdsa_signature *sig,
    const unsigned char *msghash32,
    const secp256k1_pubkey *pubkey
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3) SECP256K1_ARG_NONNULL(4);
SECP256K1_API int secp256k1_ecdsa_signature_normalize(
    const secp256k1_context* ctx,
    secp256k1_ecdsa_signature *sigout,
    const secp256k1_ecdsa_signature *sigin
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(3);
SECP256K1_API extern const secp256k1_nonce_function secp256k1_nonce_function_rfc6979;
SECP256K1_API extern const secp256k1_nonce_function secp256k1_nonce_function_default;
SECP256K1_API int secp256k1_ecdsa_sign(
    const secp256k1_context* ctx,
    secp256k1_ecdsa_signature *sig,
    const unsigned char *msghash32,
    const unsigned char *seckey,
    secp256k1_nonce_function noncefp,
    const void *ndata
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3) SECP256K1_ARG_NONNULL(4);
SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_ec_seckey_verify(
    const secp256k1_context* ctx,
    const unsigned char *seckey
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2);
SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_ec_pubkey_create(
    const secp256k1_context* ctx,
    secp256k1_pubkey *pubkey,
    const unsigned char *seckey
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3);
SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_ec_seckey_negate(
    const secp256k1_context* ctx,
    unsigned char *seckey
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2);
SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_ec_privkey_negate(
    const secp256k1_context* ctx,
    unsigned char *seckey
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2)
  SECP256K1_DEPRECATED("Use secp256k1_ec_seckey_negate instead");
SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_ec_pubkey_negate(
    const secp256k1_context* ctx,
    secp256k1_pubkey *pubkey
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2);
SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_ec_seckey_tweak_add(
    const secp256k1_context* ctx,
    unsigned char *seckey,
    const unsigned char *tweak32
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3);
SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_ec_privkey_tweak_add(
    const secp256k1_context* ctx,
    unsigned char *seckey,
    const unsigned char *tweak32
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3)
  SECP256K1_DEPRECATED("Use secp256k1_ec_seckey_tweak_add instead");
SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_ec_pubkey_tweak_add(
    const secp256k1_context* ctx,
    secp256k1_pubkey *pubkey,
    const unsigned char *tweak32
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3);
SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_ec_seckey_tweak_mul(
    const secp256k1_context* ctx,
    unsigned char *seckey,
    const unsigned char *tweak32
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3);
SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_ec_privkey_tweak_mul(
    const secp256k1_context* ctx,
    unsigned char *seckey,
    const unsigned char *tweak32
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3)
  SECP256K1_DEPRECATED("Use secp256k1_ec_seckey_tweak_mul instead");
SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_ec_pubkey_tweak_mul(
    const secp256k1_context* ctx,
    secp256k1_pubkey *pubkey,
    const unsigned char *tweak32
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3);
SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_context_randomize(
    secp256k1_context* ctx,
    const unsigned char *seed32
) SECP256K1_ARG_NONNULL(1);
SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_ec_pubkey_combine(
    const secp256k1_context* ctx,
    secp256k1_pubkey *out,
    const secp256k1_pubkey * const * ins,
    size_t n
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3);
SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_tagged_sha256(
    const secp256k1_context* ctx,
    unsigned char *hash32,
    const unsigned char *tag,
    size_t taglen,
    const unsigned char *msg,
    size_t msglen
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3) SECP256K1_ARG_NONNULL(5);
#ifdef __cplusplus
}
#endif
#endif
