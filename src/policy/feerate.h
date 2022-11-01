#ifndef LCRYP_POLICY_FEERATE_H
#define LCRYP_POLICY_FEERATE_H
#include <consensus/amount.h>
#include <serialize.h>
#include <cstdint>
#include <string>
#include <type_traits>
const std::string CURRENCY_UNIT = "LCR";
const std::string CURRENCY_ATOM = "ryp";
enum class FeeEstimateMode {
    UNSET,
    ECONOMICAL,
    CONSERVATIVE,
    LCR_KVB,
    ryp_VB,
};
class CFeeRate
{
private:
    CAmount nRypsPerK;
public:
    CFeeRate() : nRypsPerK(0) { }
    template<typename I>
    explicit CFeeRate(const I _nRypsPerK): nRypsPerK(_nRypsPerK) {
        static_assert(std::is_integral<I>::value, "CFeeRate should be used without floats");
    }
    CFeeRate(const CAmount& nFeePaid, uint32_t num_bytes);
    CAmount GetFee(uint32_t num_bytes) const;
    CAmount GetFeePerK() const { return GetFee(1000); }
    friend bool operator<(const CFeeRate& a, const CFeeRate& b) { return a.nRypsPerK < b.nRypsPerK; }
    friend bool operator>(const CFeeRate& a, const CFeeRate& b) { return a.nRypsPerK > b.nRypsPerK; }
    friend bool operator==(const CFeeRate& a, const CFeeRate& b) { return a.nRypsPerK == b.nRypsPerK; }
    friend bool operator<=(const CFeeRate& a, const CFeeRate& b) { return a.nRypsPerK <= b.nRypsPerK; }
    friend bool operator>=(const CFeeRate& a, const CFeeRate& b) { return a.nRypsPerK >= b.nRypsPerK; }
    friend bool operator!=(const CFeeRate& a, const CFeeRate& b) { return a.nRypsPerK != b.nRypsPerK; }
    CFeeRate& operator+=(const CFeeRate& a) { nRypsPerK += a.nRypsPerK; return *this; }
    std::string ToString(const FeeEstimateMode& fee_estimate_mode = FeeEstimateMode::LCR_KVB) const;
    SERIALIZE_METHODS(CFeeRate, obj) { READWRITE(obj.nRypsPerK); }
};
#endif
