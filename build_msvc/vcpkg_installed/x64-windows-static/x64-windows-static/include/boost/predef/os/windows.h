#ifndef BOOST_PREDEF_OS_WINDOWS_H
#define BOOST_PREDEF_OS_WINDOWS_H
#include <boost/predef/version_number.h>
#include <boost/predef/make.h>
#define BOOST_OS_WINDOWS BOOST_VERSION_NUMBER_NOT_AVAILABLE
#if !defined(BOOST_PREDEF_DETAIL_OS_DETECTED) && ( \
    defined(_WIN32) || defined(_WIN64) || \
    defined(__WIN32__) || defined(__TOS_WIN__) || \
    defined(__WINDOWS__) \
    )
#   undef BOOST_OS_WINDOWS
#   define BOOST_OS_WINDOWS BOOST_VERSION_NUMBER_AVAILABLE
#endif
#if BOOST_OS_WINDOWS
#   define BOOST_OS_WINDOWS_AVAILABLE
#   include <boost/predef/detail/os_detected.h>
#endif
#define BOOST_OS_WINDOWS_NAME "Microsoft Windows"
#endif
#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_OS_WINDOWS,BOOST_OS_WINDOWS_NAME)