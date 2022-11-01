#ifndef BOOST_PREDEF_ARCHITECTURE_SUPERH_H
#define BOOST_PREDEF_ARCHITECTURE_SUPERH_H
#include <boost/predef/version_number.h>
#include <boost/predef/make.h>
#define BOOST_ARCH_SH BOOST_VERSION_NUMBER_NOT_AVAILABLE
#if defined(__sh__)
#   undef BOOST_ARCH_SH
#   if !defined(BOOST_ARCH_SH) && (defined(__SH5__))
#       define BOOST_ARCH_SH BOOST_VERSION_NUMBER(5,0,0)
#   endif
#   if !defined(BOOST_ARCH_SH) && (defined(__SH4__))
#       define BOOST_ARCH_SH BOOST_VERSION_NUMBER(4,0,0)
#   endif
#   if !defined(BOOST_ARCH_SH) && (defined(__sh3__) || defined(__SH3__))
#       define BOOST_ARCH_SH BOOST_VERSION_NUMBER(3,0,0)
#   endif
#   if !defined(BOOST_ARCH_SH) && (defined(__sh2__))
#       define BOOST_ARCH_SH BOOST_VERSION_NUMBER(2,0,0)
#   endif
#   if !defined(BOOST_ARCH_SH) && (defined(__sh1__))
#       define BOOST_ARCH_SH BOOST_VERSION_NUMBER(1,0,0)
#   endif
#   if !defined(BOOST_ARCH_SH)
#       define BOOST_ARCH_SH BOOST_VERSION_NUMBER_AVAILABLE
#   endif
#endif
#if BOOST_ARCH_SH
#   define BOOST_ARCH_SH_AVAILABLE
#endif
#if BOOST_ARCH_SH
#   if BOOST_ARCH_SH >= BOOST_VERSION_NUMBER(5,0,0)
#       undef BOOST_ARCH_WORD_BITS_64
#       define BOOST_ARCH_WORD_BITS_64 BOOST_VERSION_NUMBER_AVAILABLE
#   elif BOOST_ARCH_SH >= BOOST_VERSION_NUMBER(3,0,0)
#       undef BOOST_ARCH_WORD_BITS_32
#       define BOOST_ARCH_WORD_BITS_32 BOOST_VERSION_NUMBER_AVAILABLE
#   else
#       undef BOOST_ARCH_WORD_BITS_16
#       define BOOST_ARCH_WORD_BITS_16 BOOST_VERSION_NUMBER_AVAILABLE
#   endif
#endif
#define BOOST_ARCH_SH_NAME "SuperH"
#endif
#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_ARCH_SH,BOOST_ARCH_SH_NAME)
