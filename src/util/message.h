#ifndef LCRYP_UTIL_MESSAGE_H
#define LCRYP_UTIL_MESSAGE_H
#include <uint256.h>
#include <string>
class CKey;
extern const std::string MESSAGE_MAGIC;
enum class MessageVerificationResult {
    ERR_INVALID_ADDRESS,
    ERR_ADDRESS_NO_KEY,
    ERR_MALFORMED_SIGNATURE,
    ERR_PUBKEY_NOT_RECOVERED,
    ERR_NOT_SIGNED,
    OK
};
enum class SigningResult {
    OK,
    PRIVATE_KEY_NOT_AVAILABLE,
    SIGNING_FAILED,
};
MessageVerificationResult MessageVerify(
    const std::string& address,
    const std::string& signature,
    const std::string& message);
bool MessageSign(
    const CKey& privkey,
    const std::string& message,
    std::string& signature);
uint256 MessageHash(const std::string& message);
std::string SigningResultString(const SigningResult res);
#endif
