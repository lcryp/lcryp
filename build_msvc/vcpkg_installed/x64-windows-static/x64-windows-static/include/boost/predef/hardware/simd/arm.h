#ifndef BOOST_PREDEF_HARDWARE_SIMD_ARM_H
#define BOOST_PREDEF_HARDWARE_SIMD_ARM_H
#include <boost/predef/version_number.h>
#include <boost/predef/hardware/simd/arm/versions.h>
#define BOOST_HW_SIMD_ARM BOOST_VERSION_NUMBER_NOT_AVAILABLE
#undef BOOST_HW_SIMD_ARM
#if !defined(BOOST_HW_SIMD_ARM) && (defined(__ARM_NEON__) || defined(__aarch64__) || defined (_M_ARM) || defined (_M_ARM64))
#   define BOOST_HW_SIMD_ARM BOOST_HW_SIMD_ARM_NEON_VERSION
#endif
#if !defined(BOOST_HW_SIMD_ARM)
#   define BOOST_HW_SIMD_ARM BOOST_VERSION_NUMBER_NOT_AVAILABLE
#else
#   define BOOST_HW_SIMD_ARM_AVAILABLE
#endif
#define BOOST_HW_SIMD_ARM_NAME "ARM SIMD"
#endif
#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_HW_SIMD_ARM, BOOST_HW_SIMD_ARM_NAME)
