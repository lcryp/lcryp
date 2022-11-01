#ifndef BOOST_PREDEF_COMPILER_COMEAU_H
#define BOOST_PREDEF_COMPILER_COMEAU_H
#include <boost/predef/version_number.h>
#include <boost/predef/make.h>
#define BOOST_COMP_COMO BOOST_VERSION_NUMBER_NOT_AVAILABLE
#if defined(__COMO__)
#   if !defined(BOOST_COMP_COMO_DETECTION) && defined(__COMO_VERSION__)
#       define BOOST_COMP_COMO_DETECTION BOOST_PREDEF_MAKE_0X_VRP(__COMO_VERSION__)
#   endif
#   if !defined(BOOST_COMP_COMO_DETECTION)
#       define BOOST_COMP_COMO_DETECTION BOOST_VERSION_NUMBER_AVAILABLE
#   endif
#endif
#ifdef BOOST_COMP_COMO_DETECTION
#   if defined(BOOST_PREDEF_DETAIL_COMP_DETECTED)
#       define BOOST_COMP_COMO_EMULATED BOOST_COMP_COMO_DETECTION
#   else
#       undef BOOST_COMP_COMO
#       define BOOST_COMP_COMO BOOST_COMP_COMO_DETECTION
#   endif
#   define BOOST_COMP_COMO_AVAILABLE
#   include <boost/predef/detail/comp_detected.h>
#endif
#define BOOST_COMP_COMO_NAME "Comeau C++"
#endif
#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_COMP_COMO,BOOST_COMP_COMO_NAME)
#ifdef BOOST_COMP_COMO_EMULATED
#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_COMP_COMO_EMULATED,BOOST_COMP_COMO_NAME)
#endif
