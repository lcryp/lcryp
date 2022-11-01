#ifndef LCRYP_UTIL_FEES_H
#define LCRYP_UTIL_FEES_H
#include <string>
enum class FeeEstimateMode;
enum class FeeReason;
bool FeeModeFromString(const std::string& mode_string, FeeEstimateMode& fee_estimate_mode);
std::string StringForFeeReason(FeeReason reason);
std::string FeeModes(const std::string& delimiter);
const std::string InvalidEstimateModeErrorMessage();
#endif
