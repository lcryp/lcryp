#ifndef LCRYP_UTIL_RBF_H
#define LCRYP_UTIL_RBF_H
#include <cstdint>
class CTransaction;
static constexpr uint32_t MAX_BIP125_RBF_SEQUENCE{0xfffffffd};
bool SignalsOptInRBF(const CTransaction& tx);
#endif
