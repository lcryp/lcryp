#ifndef BOOST_PREDEF_OS_MACOS_H
#define BOOST_PREDEF_OS_MACOS_H
#include <boost/predef/os/ios.h>
#include <boost/predef/version_number.h>
#include <boost/predef/make.h>
#define BOOST_OS_MACOS BOOST_VERSION_NUMBER_NOT_AVAILABLE
#if !defined(BOOST_PREDEF_DETAIL_OS_DETECTED) && ( \
    defined(macintosh) || defined(Macintosh) || \
    (defined(__APPLE__) && defined(__MACH__)) \
    )
#   undef BOOST_OS_MACOS
#   if !defined(BOOST_OS_MACOS) && defined(__APPLE__) && defined(__MACH__)
#       define BOOST_OS_MACOS BOOST_VERSION_NUMBER(10,0,0)
#   endif
#   if !defined(BOOST_OS_MACOS)
#       define BOOST_OS_MACOS BOOST_VERSION_NUMBER(9,0,0)
#   endif
#endif
#if BOOST_OS_MACOS
#   define BOOST_OS_MACOS_AVAILABLE
#   include <boost/predef/detail/os_detected.h>
#endif
#define BOOST_OS_MACOS_NAME "Mac OS"
#endif
#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_OS_MACOS,BOOST_OS_MACOS_NAME)
