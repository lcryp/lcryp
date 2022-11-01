#ifndef BOOST_PREDEF_ARCHITECTURE_E2K_H
#define BOOST_PREDEF_ARCHITECTURE_E2K_H
#include <boost/predef/version_number.h>
#include <boost/predef/make.h>
#define BOOST_ARCH_E2K BOOST_VERSION_NUMBER_NOT_AVAILABLE
#if defined(__e2k__)
#   undef BOOST_ARCH_E2K
#   if !defined(BOOST_ARCH_E2K) && defined(__iset__)
#       define BOOST_ARCH_E2K BOOST_VERSION_NUMBER(__iset__,0,0)
#   endif
#   if !defined(BOOST_ARCH_E2K)
#       define BOOST_ARCH_E2K BOOST_VERSION_NUMBER_AVAILABLE
#   endif
#endif
#if BOOST_ARCH_E2K
#   define BOOST_ARCH_E2K_AVAILABLE
#endif
#if BOOST_ARCH_E2K
#   define BOOST_ARCH_WORD_BITS_64 BOOST_VERSION_NUMBER_AVAILABLE
#endif
#define BOOST_ARCH_E2K_NAME "E2K"
#endif
#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_ARCH_E2K,BOOST_ARCH_E2K_NAME)
