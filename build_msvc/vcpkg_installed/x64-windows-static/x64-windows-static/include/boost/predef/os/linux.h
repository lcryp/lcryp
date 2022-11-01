#ifndef BOOST_PREDEF_OS_LINUX_H
#define BOOST_PREDEF_OS_LINUX_H
#include <boost/predef/version_number.h>
#include <boost/predef/make.h>
#define BOOST_OS_LINUX BOOST_VERSION_NUMBER_NOT_AVAILABLE
#if !defined(BOOST_PREDEF_DETAIL_OS_DETECTED) && ( \
    defined(linux) || defined(__linux) || \
    defined(__linux__) || defined(__gnu_linux__) \
    )
#   undef BOOST_OS_LINUX
#   define BOOST_OS_LINUX BOOST_VERSION_NUMBER_AVAILABLE
#endif
#if BOOST_OS_LINUX
#   define BOOST_OS_LINUX_AVAILABLE
#   include <boost/predef/detail/os_detected.h>
#endif
#define BOOST_OS_LINUX_NAME "Linux"
#endif
#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_OS_LINUX,BOOST_OS_LINUX_NAME)
