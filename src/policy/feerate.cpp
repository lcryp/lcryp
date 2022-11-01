#include <consensus/amount.h>
#include <policy/feerate.h>
#include <tinyformat.h>
#include <cmath>
CFeeRate::CFeeRate(const CAmount& nFeePaid, uint32_t num_bytes)
{
    const int64_t nSize{num_bytes};
    if (nSize > 0) {
        nRypsPerK = nFeePaid * 1000 / nSize;
    } else {
        nRypsPerK = 0;
    }
}
CAmount CFeeRate::GetFee(uint32_t num_bytes) const
{
    const int64_t nSize{num_bytes};
    CAmount nFee{static_cast<CAmount>(std::ceil(nRypsPerK * nSize / 1000.0))};
    if (nFee == 0 && nSize != 0) {
        if (nRypsPerK > 0) nFee = CAmount(1);
        if (nRypsPerK < 0) nFee = CAmount(-1);
    }
    return nFee;
}
std::string CFeeRate::ToString(const FeeEstimateMode& fee_estimate_mode) const
{
    switch (fee_estimate_mode) {
    case FeeEstimateMode::ryp_VB: return strprintf("%d.%03d %s/vB", nRypsPerK / 1000, nRypsPerK % 1000, CURRENCY_ATOM);
    default:                      return strprintf("%d.%08d %s/kvB", nRypsPerK / COIN, nRypsPerK % COIN, CURRENCY_UNIT);
    }
}
