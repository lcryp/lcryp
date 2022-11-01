#ifndef BOOST_PREDEF_ARCHITECTURE_CONVEX_H
#define BOOST_PREDEF_ARCHITECTURE_CONVEX_H
#include <boost/predef/version_number.h>
#include <boost/predef/make.h>
#define BOOST_ARCH_CONVEX BOOST_VERSION_NUMBER_NOT_AVAILABLE
#if defined(__convex__)
#   undef BOOST_ARCH_CONVEX
#   if !defined(BOOST_ARCH_CONVEX) && defined(__convex_c1__)
#       define BOOST_ARCH_CONVEX BOOST_VERSION_NUMBER(1,0,0)
#   endif
#   if !defined(BOOST_ARCH_CONVEX) && defined(__convex_c2__)
#       define BOOST_ARCH_CONVEX BOOST_VERSION_NUMBER(2,0,0)
#   endif
#   if !defined(BOOST_ARCH_CONVEX) && defined(__convex_c32__)
#       define BOOST_ARCH_CONVEX BOOST_VERSION_NUMBER(3,2,0)
#   endif
#   if !defined(BOOST_ARCH_CONVEX) && defined(__convex_c34__)
#       define BOOST_ARCH_CONVEX BOOST_VERSION_NUMBER(3,4,0)
#   endif
#   if !defined(BOOST_ARCH_CONVEX) && defined(__convex_c38__)
#       define BOOST_ARCH_CONVEX BOOST_VERSION_NUMBER(3,8,0)
#   endif
#   if !defined(BOOST_ARCH_CONVEX)
#       define BOOST_ARCH_CONVEX BOOST_VERSION_NUMBER_AVAILABLE
#   endif
#endif
#if BOOST_ARCH_CONVEX
#   define BOOST_ARCH_CONVEX_AVAILABLE
#endif
#if BOOST_ARCH_CONVEX
#   undef BOOST_ARCH_WORD_BITS_32
#   define BOOST_ARCH_WORD_BITS_32 BOOST_VERSION_NUMBER_AVAILABLE
#endif
#define BOOST_ARCH_CONVEX_NAME "Convex Computer"
#endif
#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_ARCH_CONVEX,BOOST_ARCH_CONVEX_NAME)
