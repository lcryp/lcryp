#ifndef BOOST_PREDEF_ARCHITECTURE_SPARC_H
#define BOOST_PREDEF_ARCHITECTURE_SPARC_H
#include <boost/predef/version_number.h>
#include <boost/predef/make.h>
#define BOOST_ARCH_SPARC BOOST_VERSION_NUMBER_NOT_AVAILABLE
#if defined(__sparc__) || defined(__sparc)
#   undef BOOST_ARCH_SPARC
#   if !defined(BOOST_ARCH_SPARC) && (defined(__sparcv9) || defined(__sparc_v9__))
#       define BOOST_ARCH_SPARC BOOST_VERSION_NUMBER(9,0,0)
#   endif
#   if !defined(BOOST_ARCH_SPARC) && (defined(__sparcv8) || defined(__sparc_v8__))
#       define BOOST_ARCH_SPARC BOOST_VERSION_NUMBER(8,0,0)
#   endif
#   if !defined(BOOST_ARCH_SPARC)
#       define BOOST_ARCH_SPARC BOOST_VERSION_NUMBER_AVAILABLE
#   endif
#endif
#if BOOST_ARCH_SPARC
#   define BOOST_ARCH_SPARC_AVAILABLE
#endif
#if BOOST_ARCH_SPARC
#   if BOOST_ARCH_SPARC >= BOOST_VERSION_NUMBER(9,0,0)
#       undef BOOST_ARCH_WORD_BITS_64
#       define BOOST_ARCH_WORD_BITS_64 BOOST_VERSION_NUMBER_AVAILABLE
#   else
#       undef BOOST_ARCH_WORD_BITS_32
#       define BOOST_ARCH_WORD_BITS_32 BOOST_VERSION_NUMBER_AVAILABLE
#   endif
#endif
#define BOOST_ARCH_SPARC_NAME "SPARC"
#endif
#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_ARCH_SPARC,BOOST_ARCH_SPARC_NAME)
