#ifndef BOOST_PREDEF_ARCHITECTURE_PPC_H
#define BOOST_PREDEF_ARCHITECTURE_PPC_H
#include <boost/predef/version_number.h>
#include <boost/predef/make.h>
#define BOOST_ARCH_PPC BOOST_VERSION_NUMBER_NOT_AVAILABLE
#if defined(__powerpc) || defined(__powerpc__) || defined(__powerpc64__) || \
    defined(__POWERPC__) || defined(__ppc__) || defined(__ppc64__) || \
    defined(__PPC__) || defined(__PPC64__) || \
    defined(_M_PPC) || defined(_ARCH_PPC) || defined(_ARCH_PPC64) || \
    defined(__PPCGECKO__) || defined(__PPCBROADWAY__) || \
    defined(_XENON) || \
    defined(__ppc)
#   undef BOOST_ARCH_PPC
#   if !defined (BOOST_ARCH_PPC) && (defined(__ppc601__) || defined(_ARCH_601))
#       define BOOST_ARCH_PPC BOOST_VERSION_NUMBER(6,1,0)
#   endif
#   if !defined (BOOST_ARCH_PPC) && (defined(__ppc603__) || defined(_ARCH_603))
#       define BOOST_ARCH_PPC BOOST_VERSION_NUMBER(6,3,0)
#   endif
#   if !defined (BOOST_ARCH_PPC) && (defined(__ppc604__) || defined(__ppc604__))
#       define BOOST_ARCH_PPC BOOST_VERSION_NUMBER(6,4,0)
#   endif
#   if !defined (BOOST_ARCH_PPC)
#       define BOOST_ARCH_PPC BOOST_VERSION_NUMBER_AVAILABLE
#   endif
#endif
#if BOOST_ARCH_PPC
#   define BOOST_ARCH_PPC_AVAILABLE
#endif
#define BOOST_ARCH_PPC_NAME "PowerPC"
#define BOOST_ARCH_PPC_64 BOOST_VERSION_NUMBER_NOT_AVAILABLE
#if defined(__powerpc64__) || defined(__ppc64__) || defined(__PPC64__) || \
    defined(_ARCH_PPC64)
#   undef BOOST_ARCH_PPC_64
#   define BOOST_ARCH_PPC_64 BOOST_VERSION_NUMBER_AVAILABLE
#endif
#if BOOST_ARCH_PPC_64
#   define BOOST_ARCH_PPC_64_AVAILABLE
#endif
#define BOOST_ARCH_PPC_64_NAME "PowerPC64"
#if BOOST_ARCH_PPC_64
#   undef BOOST_ARCH_WORD_BITS_64
#   define BOOST_ARCH_WORD_BITS_64 BOOST_VERSION_NUMBER_AVAILABLE
#else
#   undef BOOST_ARCH_WORD_BITS_32
#   define BOOST_ARCH_WORD_BITS_32 BOOST_VERSION_NUMBER_AVAILABLE
#endif
#endif
#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_ARCH_PPC,BOOST_ARCH_PPC_NAME)
#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_ARCH_PPC_64,BOOST_ARCH_PPC_64_NAME)
