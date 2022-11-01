#ifndef BOOST_PREDEF_LANGUAGE_STDC_H
#define BOOST_PREDEF_LANGUAGE_STDC_H
#include <boost/predef/version_number.h>
#include <boost/predef/make.h>
#define BOOST_LANG_STDC BOOST_VERSION_NUMBER_NOT_AVAILABLE
#if defined(__STDC__)
#   undef BOOST_LANG_STDC
#   if defined(__STDC_VERSION__)
#       if (__STDC_VERSION__ > 100)
#           define BOOST_LANG_STDC BOOST_PREDEF_MAKE_YYYYMM(__STDC_VERSION__)
#       else
#           define BOOST_LANG_STDC BOOST_VERSION_NUMBER_AVAILABLE
#       endif
#   else
#       define BOOST_LANG_STDC BOOST_VERSION_NUMBER_AVAILABLE
#   endif
#endif
#if BOOST_LANG_STDC
#   define BOOST_LANG_STDC_AVAILABLE
#endif
#define BOOST_LANG_STDC_NAME "Standard C"
#endif
#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_LANG_STDC,BOOST_LANG_STDC_NAME)
