#include <boost/predef/hardware/simd/x86.h>
#include <boost/predef/hardware/simd/x86_amd.h>
#include <boost/predef/hardware/simd/arm.h>
#include <boost/predef/hardware/simd/ppc.h>
#ifndef BOOST_PREDEF_HARDWARE_SIMD_H
#define BOOST_PREDEF_HARDWARE_SIMD_H
#include <boost/predef/version_number.h>
#if defined(BOOST_HW_SIMD_ARM_AVAILABLE) && defined(BOOST_HW_SIMD_PPC_AVAILABLE) ||\
    defined(BOOST_HW_SIMD_ARM_AVAILABLE) && defined(BOOST_HW_SIMD_X86_AVAILABLE) ||\
    defined(BOOST_HW_SIMD_PPC_AVAILABLE) && defined(BOOST_HW_SIMD_X86_AVAILABLE)
#   error "Multiple SIMD architectures detected, this cannot happen!"
#endif
#if defined(BOOST_HW_SIMD_X86_AVAILABLE) && defined(BOOST_HW_SIMD_X86_AMD_AVAILABLE)
#   if BOOST_HW_SIMD_X86 >= BOOST_HW_SIMD_X86_AMD
#      define BOOST_HW_SIMD BOOST_HW_SIMD_X86
#   else
#      define BOOST_HW_SIMD BOOST_HW_SIMD_X86_AMD
#   endif
#endif
#if !defined(BOOST_HW_SIMD)
#   if defined(BOOST_HW_SIMD_X86_AVAILABLE)
#      define BOOST_HW_SIMD BOOST_HW_SIMD_X86
#   endif
#   if defined(BOOST_HW_SIMD_X86_AMD_AVAILABLE)
#      define BOOST_HW_SIMD BOOST_HW_SIMD_X86_AMD
#   endif
#endif
#if defined(BOOST_HW_SIMD_ARM_AVAILABLE)
#   define BOOST_HW_SIMD BOOST_HW_SIMD_ARM
#endif
#if defined(BOOST_HW_SIMD_PPC_AVAILABLE)
#   define BOOST_HW_SIMD BOOST_HW_SIMD_PPC
#endif
#if defined(BOOST_HW_SIMD)
#   define BOOST_HW_SIMD_AVAILABLE
#else
#   define BOOST_HW_SIMD BOOST_VERSION_NUMBER_NOT_AVAILABLE
#endif
#define BOOST_HW_SIMD_NAME "Hardware SIMD"
#endif
#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_HW_SIMD, BOOST_HW_SIMD_NAME)
