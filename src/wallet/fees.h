#ifndef LCRYP_WALLET_FEES_H
#define LCRYP_WALLET_FEES_H
#include <consensus/amount.h>
class CFeeRate;
struct FeeCalculation;
namespace wallet {
class CCoinControl;
class CWallet;
CAmount GetRequiredFee(const CWallet& wallet, unsigned int nTxBytes);
CAmount GetMinimumFee(const CWallet& wallet, unsigned int nTxBytes, const CCoinControl& coin_control, FeeCalculation* feeCalc);
CFeeRate GetRequiredFeeRate(const CWallet& wallet);
CFeeRate GetMinimumFeeRate(const CWallet& wallet, const CCoinControl& coin_control, FeeCalculation* feeCalc);
CFeeRate GetDiscardRate(const CWallet& wallet);
}
#endif
