#ifndef BOOST_PREDEF_LANGUAGE_STDCPP_H
#define BOOST_PREDEF_LANGUAGE_STDCPP_H
#include <boost/predef/version_number.h>
#include <boost/predef/make.h>
#define BOOST_LANG_STDCPP BOOST_VERSION_NUMBER_NOT_AVAILABLE
#if defined(__cplusplus)
#   undef BOOST_LANG_STDCPP
#   if (__cplusplus > 100)
#       define BOOST_LANG_STDCPP BOOST_PREDEF_MAKE_YYYYMM(__cplusplus)
#   else
#       define BOOST_LANG_STDCPP BOOST_VERSION_NUMBER_AVAILABLE
#   endif
#endif
#if BOOST_LANG_STDCPP
#   define BOOST_LANG_STDCPP_AVAILABLE
#endif
#define BOOST_LANG_STDCPP_NAME "Standard C++"
#define BOOST_LANG_STDCPPCLI BOOST_VERSION_NUMBER_NOT_AVAILABLE
#if defined(__cplusplus_cli)
#   undef BOOST_LANG_STDCPPCLI
#   if (__cplusplus_cli > 100)
#       define BOOST_LANG_STDCPPCLI BOOST_PREDEF_MAKE_YYYYMM(__cplusplus_cli)
#   else
#       define BOOST_LANG_STDCPPCLI BOOST_VERSION_NUMBER_AVAILABLE
#   endif
#endif
#if BOOST_LANG_STDCPPCLI
#   define BOOST_LANG_STDCPPCLI_AVAILABLE
#endif
#define BOOST_LANG_STDCPPCLI_NAME "Standard C++/CLI"
#define BOOST_LANG_STDECPP BOOST_VERSION_NUMBER_NOT_AVAILABLE
#if defined(__embedded_cplusplus)
#   undef BOOST_LANG_STDECPP
#   define BOOST_LANG_STDECPP BOOST_VERSION_NUMBER_AVAILABLE
#endif
#if BOOST_LANG_STDECPP
#   define BOOST_LANG_STDECPP_AVAILABLE
#endif
#define BOOST_LANG_STDECPP_NAME "Standard Embedded C++"
#endif
#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_LANG_STDCPP,BOOST_LANG_STDCPP_NAME)
#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_LANG_STDCPPCLI,BOOST_LANG_STDCPPCLI_NAME)
#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_LANG_STDECPP,BOOST_LANG_STDECPP_NAME)
