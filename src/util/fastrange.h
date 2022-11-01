#ifndef LCRYP_UTIL_FASTRANGE_H
#define LCRYP_UTIL_FASTRANGE_H
#include <cstdint>
static inline uint32_t FastRange32(uint32_t x, uint32_t n)
{
    return (uint64_t{x} * n) >> 32;
}
static inline uint64_t FastRange64(uint64_t x, uint64_t n)
{
#ifdef __SIZEOF_INT128__
    return (static_cast<unsigned __int128>(x) * static_cast<unsigned __int128>(n)) >> 64;
#else
    const uint64_t x_hi = x >> 32;
    const uint64_t x_lo = x & 0xFFFFFFFF;
    const uint64_t n_hi = n >> 32;
    const uint64_t n_lo = n & 0xFFFFFFFF;
    const uint64_t ac = x_hi * n_hi;
    const uint64_t ad = x_hi * n_lo;
    const uint64_t bc = x_lo * n_hi;
    const uint64_t bd = x_lo * n_lo;
    const uint64_t mid34 = (bd >> 32) + (bc & 0xFFFFFFFF) + (ad & 0xFFFFFFFF);
    const uint64_t upper64 = ac + (bc >> 32) + (ad >> 32) + (mid34 >> 32);
    return upper64;
#endif
}
#endif
