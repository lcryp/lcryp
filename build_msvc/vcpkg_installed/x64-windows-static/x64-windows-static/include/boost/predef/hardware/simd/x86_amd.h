#ifndef BOOST_PREDEF_HARDWARE_SIMD_X86_AMD_H
#define BOOST_PREDEF_HARDWARE_SIMD_X86_AMD_H
#include <boost/predef/version_number.h>
#include <boost/predef/hardware/simd/x86_amd/versions.h>
#define BOOST_HW_SIMD_X86_AMD BOOST_VERSION_NUMBER_NOT_AVAILABLE
#undef BOOST_HW_SIMD_X86_AMD
#if !defined(BOOST_HW_SIMD_X86_AMD) && defined(__XOP__)
#   define BOOST_HW_SIMD_X86_AMD BOOST_HW_SIMD_X86_AMD_XOP_VERSION
#endif
#if !defined(BOOST_HW_SIMD_X86_AMD) && defined(__FMA4__)
#   define BOOST_HW_SIMD_X86_AMD BOOST_HW_SIMD_X86_AMD_FMA4_VERSION
#endif
#if !defined(BOOST_HW_SIMD_X86_AMD) && defined(__SSE4A__)
#   define BOOST_HW_SIMD_X86_AMD BOOST_HW_SIMD_X86_AMD_SSE4A_VERSION
#endif
#if !defined(BOOST_HW_SIMD_X86_AMD)
#   define BOOST_HW_SIMD_X86_AMD BOOST_VERSION_NUMBER_NOT_AVAILABLE
#else
#   include <boost/predef/hardware/simd/x86.h>
#   if BOOST_HW_SIMD_X86 > BOOST_HW_SIMD_X86_AMD
#      undef BOOST_HW_SIMD_X86_AMD
#      define BOOST_HW_SIMD_X86_AMD BOOST_HW_SIMD_X86
#   endif
#   define BOOST_HW_SIMD_X86_AMD_AVAILABLE
#endif
#define BOOST_HW_SIMD_X86_AMD_NAME "x86 (AMD) SIMD"
#endif
#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_HW_SIMD_X86_AMD, BOOST_HW_SIMD_X86_AMD_NAME)
