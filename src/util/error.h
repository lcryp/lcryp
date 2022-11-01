#ifndef LCRYP_UTIL_ERROR_H
#define LCRYP_UTIL_ERROR_H
#include <string>
struct bilingual_str;
enum class TransactionError {
    OK,
    MISSING_INPUTS,
    ALREADY_IN_CHAIN,
    P2P_DISABLED,
    MEMPOOL_REJECTED,
    MEMPOOL_ERROR,
    INVALID_PSBT,
    PSBT_MISMATCH,
    SIGHASH_MISMATCH,
    MAX_FEE_EXCEEDED,
    EXTERNAL_SIGNER_NOT_FOUND,
    EXTERNAL_SIGNER_FAILED,
    INVALID_PACKAGE,
};
bilingual_str TransactionErrorString(const TransactionError error);
bilingual_str ResolveErrMsg(const std::string& optname, const std::string& strBind);
bilingual_str AmountHighWarn(const std::string& optname);
bilingual_str AmountErrMsg(const std::string& optname, const std::string& strValue);
#endif
