#ifndef BOOST_PREDEF_COMPILER_MICROTEC_H
#define BOOST_PREDEF_COMPILER_MICROTEC_H
#include <boost/predef/version_number.h>
#include <boost/predef/make.h>
#define BOOST_COMP_MRI BOOST_VERSION_NUMBER_NOT_AVAILABLE
#if defined(_MRI)
#   define BOOST_COMP_MRI_DETECTION BOOST_VERSION_NUMBER_AVAILABLE
#endif
#ifdef BOOST_COMP_MRI_DETECTION
#   if defined(BOOST_PREDEF_DETAIL_COMP_DETECTED)
#       define BOOST_COMP_MRI_EMULATED BOOST_COMP_MRI_DETECTION
#   else
#       undef BOOST_COMP_MRI
#       define BOOST_COMP_MRI BOOST_COMP_MRI_DETECTION
#   endif
#   define BOOST_COMP_MRI_AVAILABLE
#   include <boost/predef/detail/comp_detected.h>
#endif
#define BOOST_COMP_MRI_NAME "Microtec C/C++"
#endif
#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_COMP_MRI,BOOST_COMP_MRI_NAME)
#ifdef BOOST_COMP_MRI_EMULATED
#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_COMP_MRI_EMULATED,BOOST_COMP_MRI_NAME)
#endif
