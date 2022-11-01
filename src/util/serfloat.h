#ifndef LCRYP_UTIL_SERFLOAT_H
#define LCRYP_UTIL_SERFLOAT_H
#include <cstdint>
uint64_t EncodeDouble(double f) noexcept;
double DecodeDouble(uint64_t v) noexcept;
#endif
