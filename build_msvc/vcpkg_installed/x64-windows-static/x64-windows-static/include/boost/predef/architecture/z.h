#ifndef BOOST_PREDEF_ARCHITECTURE_Z_H
#define BOOST_PREDEF_ARCHITECTURE_Z_H
#include <boost/predef/version_number.h>
#include <boost/predef/make.h>
#define BOOST_ARCH_Z BOOST_VERSION_NUMBER_NOT_AVAILABLE
#if defined(__SYSC_ZARCH__)
#   undef BOOST_ARCH_Z
#   define BOOST_ARCH_Z BOOST_VERSION_NUMBER_AVAILABLE
#endif
#if BOOST_ARCH_Z
#   define BOOST_ARCH_Z_AVAILABLE
#endif
#if BOOST_ARCH_Z
#   undef BOOST_ARCH_WORD_BITS_64
#   define BOOST_ARCH_WORD_BITS_64 BOOST_VERSION_NUMBER_AVAILABLE
#endif
#define BOOST_ARCH_Z_NAME "z/Architecture"
#endif
#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_ARCH_Z,BOOST_ARCH_Z_NAME)
