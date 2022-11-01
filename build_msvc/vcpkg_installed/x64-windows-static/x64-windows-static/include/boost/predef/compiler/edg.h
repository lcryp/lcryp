#ifndef BOOST_PREDEF_COMPILER_EDG_H
#define BOOST_PREDEF_COMPILER_EDG_H
#include <boost/predef/version_number.h>
#include <boost/predef/make.h>
#define BOOST_COMP_EDG BOOST_VERSION_NUMBER_NOT_AVAILABLE
#if defined(__EDG__)
#   define BOOST_COMP_EDG_DETECTION BOOST_PREDEF_MAKE_10_VRR(__EDG_VERSION__)
#endif
#ifdef BOOST_COMP_EDG_DETECTION
#   if defined(BOOST_PREDEF_DETAIL_COMP_DETECTED)
#       define BOOST_COMP_EDG_EMULATED BOOST_COMP_EDG_DETECTION
#   else
#       undef BOOST_COMP_EDG
#       define BOOST_COMP_EDG BOOST_COMP_EDG_DETECTION
#   endif
#   define BOOST_COMP_EDG_AVAILABLE
#   include <boost/predef/detail/comp_detected.h>
#endif
#define BOOST_COMP_EDG_NAME "EDG C++ Frontend"
#endif
#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_COMP_EDG,BOOST_COMP_EDG_NAME)
#ifdef BOOST_COMP_EDG_EMULATED
#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_COMP_EDG_EMULATED,BOOST_COMP_EDG_NAME)
#endif