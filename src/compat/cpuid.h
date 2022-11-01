#ifndef LCRYP_COMPAT_CPUID_H
#define LCRYP_COMPAT_CPUID_H
#if defined(__x86_64__) || defined(__amd64__) || defined(__i386__)
#define HAVE_GETCPUID
#include <cpuid.h>
#include <cstdint>
void static inline GetCPUID(uint32_t leaf, uint32_t subleaf, uint32_t& a, uint32_t& b, uint32_t& c, uint32_t& d)
{
#ifdef __GNUC__
    __cpuid_count(leaf, subleaf, a, b, c, d);
#else
  __asm__ ("cpuid" : "=a"(a), "=b"(b), "=c"(c), "=d"(d) : "0"(leaf), "2"(subleaf));
#endif
}
#endif
#endif
