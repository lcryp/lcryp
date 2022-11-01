#ifndef LCRYP_CONSENSUS_AMOUNT_H
#define LCRYP_CONSENSUS_AMOUNT_H
#include <cstdint>
typedef int64_t CAmount;
static constexpr CAmount COIN = 100000000;
static constexpr CAmount MAX_MONEY = 22024000.0 * COIN;
inline bool MoneyRange(const CAmount& nValue) { return (nValue >= 0 && nValue <= MAX_MONEY); }
#endif
