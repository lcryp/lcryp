#ifndef BOOST_PREDEF_HARDWARE_SIMD_PPC_H
#define BOOST_PREDEF_HARDWARE_SIMD_PPC_H
#include <boost/predef/version_number.h>
#include <boost/predef/hardware/simd/ppc/versions.h>
#define BOOST_HW_SIMD_PPC BOOST_VERSION_NUMBER_NOT_AVAILABLE
#undef BOOST_HW_SIMD_PPC
#if !defined(BOOST_HW_SIMD_PPC) && defined(__VECTOR4DOUBLE__)
#   define BOOST_HW_SIMD_PPC BOOST_HW_SIMD_PPC_QPX_VERSION
#endif
#if !defined(BOOST_HW_SIMD_PPC) && defined(__VSX__)
#   define BOOST_HW_SIMD_PPC BOOST_HW_SIMD_PPC_VSX_VERSION
#endif
#if !defined(BOOST_HW_SIMD_PPC) && (defined(__ALTIVEC__) || defined(__VEC__))
#   define BOOST_HW_SIMD_PPC BOOST_HW_SIMD_PPC_VMX_VERSION
#endif
#if !defined(BOOST_HW_SIMD_PPC)
#   define BOOST_HW_SIMD_PPC BOOST_VERSION_NUMBER_NOT_AVAILABLE
#else
#   define BOOST_HW_SIMD_PPC_AVAILABLE
#endif
#define BOOST_HW_SIMD_PPC_NAME "PPC SIMD"
#endif
#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_HW_SIMD_PPC, BOOST_HW_SIMD_PPC_NAME)
