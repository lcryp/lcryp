#ifndef LCRYP_SCRIPT_LCRYPCONSENSUS_H
#define LCRYP_SCRIPT_LCRYPCONSENSUS_H
#include <stdint.h>
#if defined(BUILD_LCRYP_INTERNAL) && defined(HAVE_CONFIG_H)
#include <config/lcryp-config.h>
  #if defined(_WIN32)
    #if defined(HAVE_DLLEXPORT_ATTRIBUTE)
      #define EXPORT_SYMBOL __declspec(dllexport)
    #else
      #define EXPORT_SYMBOL
    #endif
  #elif defined(HAVE_DEFAULT_VISIBILITY_ATTRIBUTE)
    #define EXPORT_SYMBOL __attribute__ ((visibility ("default")))
  #endif
#elif defined(MSC_VER) && !defined(STATIC_LIBLCRYPCONSENSUS)
  #define EXPORT_SYMBOL __declspec(dllimport)
#endif
#ifndef EXPORT_SYMBOL
  #define EXPORT_SYMBOL
#endif
#ifdef __cplusplus
extern "C" {
#endif
#define LCRYPCONSENSUS_API_VER 1
typedef enum lcrypconsensus_error_t
{
    lcrypconsensus_ERR_OK = 0,
    lcrypconsensus_ERR_TX_INDEX,
    lcrypconsensus_ERR_TX_SIZE_MISMATCH,
    lcrypconsensus_ERR_TX_DESERIALIZE,
    lcrypconsensus_ERR_AMOUNT_REQUIRED,
    lcrypconsensus_ERR_INVALID_FLAGS,
} lcrypconsensus_error;
enum
{
    lcrypconsensus_SCRIPT_FLAGS_VERIFY_NONE                = 0,
    lcrypconsensus_SCRIPT_FLAGS_VERIFY_P2SH                = (1U << 0),
    lcrypconsensus_SCRIPT_FLAGS_VERIFY_DERSIG              = (1U << 2),
    lcrypconsensus_SCRIPT_FLAGS_VERIFY_NULLDUMMY           = (1U << 4),
    lcrypconsensus_SCRIPT_FLAGS_VERIFY_CHECKLOCKTIMEVERIFY = (1U << 9),
    lcrypconsensus_SCRIPT_FLAGS_VERIFY_CHECKSEQUENCEVERIFY = (1U << 10),
    lcrypconsensus_SCRIPT_FLAGS_VERIFY_WITNESS             = (1U << 11),
    lcrypconsensus_SCRIPT_FLAGS_VERIFY_ALL                 = lcrypconsensus_SCRIPT_FLAGS_VERIFY_P2SH | lcrypconsensus_SCRIPT_FLAGS_VERIFY_DERSIG |
                                                               lcrypconsensus_SCRIPT_FLAGS_VERIFY_NULLDUMMY | lcrypconsensus_SCRIPT_FLAGS_VERIFY_CHECKLOCKTIMEVERIFY |
                                                               lcrypconsensus_SCRIPT_FLAGS_VERIFY_CHECKSEQUENCEVERIFY | lcrypconsensus_SCRIPT_FLAGS_VERIFY_WITNESS
};
EXPORT_SYMBOL int lcrypconsensus_verify_script(const unsigned char *scriptPubKey, unsigned int scriptPubKeyLen,
                                                 const unsigned char *txTo        , unsigned int txToLen,
                                                 unsigned int nIn, unsigned int flags, lcrypconsensus_error* err);
EXPORT_SYMBOL int lcrypconsensus_verify_script_with_amount(const unsigned char *scriptPubKey, unsigned int scriptPubKeyLen, int64_t amount,
                                    const unsigned char *txTo        , unsigned int txToLen,
                                    unsigned int nIn, unsigned int flags, lcrypconsensus_error* err);
EXPORT_SYMBOL unsigned int lcrypconsensus_version();
#ifdef __cplusplus
}
#endif
#undef EXPORT_SYMBOL
#endif
