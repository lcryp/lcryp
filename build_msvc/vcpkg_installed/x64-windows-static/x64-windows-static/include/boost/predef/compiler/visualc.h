#ifndef BOOST_PREDEF_COMPILER_VISUALC_H
#define BOOST_PREDEF_COMPILER_VISUALC_H
#include <boost/predef/compiler/clang.h>
#include <boost/predef/version_number.h>
#include <boost/predef/make.h>
#define BOOST_COMP_MSVC BOOST_VERSION_NUMBER_NOT_AVAILABLE
#if defined(_MSC_VER)
#   if !defined (_MSC_FULL_VER)
#       define BOOST_COMP_MSVC_BUILD 0
#   else
#       if _MSC_FULL_VER / 10000 == _MSC_VER
#           define BOOST_COMP_MSVC_BUILD (_MSC_FULL_VER % 10000)
#       elif _MSC_FULL_VER / 100000 == _MSC_VER
#           define BOOST_COMP_MSVC_BUILD (_MSC_FULL_VER % 100000)
#       else
#           error "Cannot determine build number from _MSC_FULL_VER"
#       endif
#   endif
#   if (_MSC_VER > 1900)
#       define BOOST_COMP_MSVC_DETECTION BOOST_VERSION_NUMBER(\
            _MSC_VER/100,\
            _MSC_VER%100,\
            BOOST_COMP_MSVC_BUILD)
#   elif (_MSC_VER >= 1900)
#       define BOOST_COMP_MSVC_DETECTION BOOST_VERSION_NUMBER(\
            _MSC_VER/100-5,\
            _MSC_VER%100,\
            BOOST_COMP_MSVC_BUILD)
#   else
#       define BOOST_COMP_MSVC_DETECTION BOOST_VERSION_NUMBER(\
            _MSC_VER/100-6,\
            _MSC_VER%100,\
            BOOST_COMP_MSVC_BUILD)
#   endif
#endif
#ifdef BOOST_COMP_MSVC_DETECTION
#   if defined(BOOST_PREDEF_DETAIL_COMP_DETECTED)
#       define BOOST_COMP_MSVC_EMULATED BOOST_COMP_MSVC_DETECTION
#   else
#       undef BOOST_COMP_MSVC
#       define BOOST_COMP_MSVC BOOST_COMP_MSVC_DETECTION
#   endif
#   define BOOST_COMP_MSVC_AVAILABLE
#   include <boost/predef/detail/comp_detected.h>
#endif
#define BOOST_COMP_MSVC_NAME "Microsoft Visual C/C++"
#endif
#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_COMP_MSVC,BOOST_COMP_MSVC_NAME)
#ifdef BOOST_COMP_MSVC_EMULATED
#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_COMP_MSVC_EMULATED,BOOST_COMP_MSVC_NAME)
#endif
