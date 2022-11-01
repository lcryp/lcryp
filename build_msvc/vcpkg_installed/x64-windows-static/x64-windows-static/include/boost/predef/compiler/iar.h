#ifndef BOOST_PREDEF_COMPILER_IAR_H
#define BOOST_PREDEF_COMPILER_IAR_H
#include <boost/predef/version_number.h>
#include <boost/predef/make.h>
#define BOOST_COMP_IAR BOOST_VERSION_NUMBER_NOT_AVAILABLE
#if defined(__IAR_SYSTEMS_ICC__)
#   define BOOST_COMP_IAR_DETECTION BOOST_PREDEF_MAKE_10_VVRR(__VER__)
#endif
#ifdef BOOST_COMP_IAR_DETECTION
#   if defined(BOOST_PREDEF_DETAIL_COMP_DETECTED)
#       define BOOST_COMP_IAR_EMULATED BOOST_COMP_IAR_DETECTION
#   else
#       undef BOOST_COMP_IAR
#       define BOOST_COMP_IAR BOOST_COMP_IAR_DETECTION
#   endif
#   define BOOST_COMP_IAR_AVAILABLE
#   include <boost/predef/detail/comp_detected.h>
#endif
#define BOOST_COMP_IAR_NAME "IAR C/C++"
#endif
#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_COMP_IAR,BOOST_COMP_IAR_NAME)
#ifdef BOOST_COMP_IAR_EMULATED
#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_COMP_IAR_EMULATED,BOOST_COMP_IAR_NAME)
#endif
