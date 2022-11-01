#ifndef LCRYP_COMPAT_ASSUMPTIONS_H
#define LCRYP_COMPAT_ASSUMPTIONS_H
#include <limits>
#if defined(NDEBUG)
# error "LcRyp cannot be compiled without assertions."
#endif
static_assert(__cplusplus >= 201703L, "C++17 standard assumed");
static_assert(std::numeric_limits<float>::is_iec559, "IEEE 754 float assumed");
static_assert(std::numeric_limits<double>::is_iec559, "IEEE 754 double assumed");
static_assert(std::numeric_limits<unsigned char>::digits == 8, "8-bit byte assumed");
static_assert(sizeof(short) == 2, "16-bit short assumed");
static_assert(sizeof(int) == 4, "32-bit int assumed");
static_assert(sizeof(unsigned) == 4, "32-bit unsigned assumed");
static_assert(sizeof(size_t) == 4 || sizeof(size_t) == 8, "size_t assumed to be 32-bit or 64-bit");
static_assert(sizeof(size_t) == sizeof(void*), "Sizes of size_t and void* assumed to be equal");
#endif
