#ifndef BOOST_PREDEF_PLAT_WINDOWS_UWP_H
#define BOOST_PREDEF_PLAT_WINDOWS_UWP_H
#include <boost/predef/make.h>
#include <boost/predef/os/windows.h>
#include <boost/predef/version_number.h>
#define BOOST_PLAT_WINDOWS_UWP BOOST_VERSION_NUMBER_NOT_AVAILABLE
#define BOOST_PLAT_WINDOWS_SDK_VERSION BOOST_VERSION_NUMBER_NOT_AVAILABLE
#if BOOST_OS_WINDOWS
#if !defined(__MINGW32__) && !defined(_WIN32_WCE) && !defined(__WINE__)
#   include <ntverp.h>
#   undef BOOST_PLAT_WINDOWS_SDK_VERSION
#   define BOOST_PLAT_WINDOWS_SDK_VERSION BOOST_VERSION_NUMBER(0, 0, VER_PRODUCTBUILD)
#endif
#if ((BOOST_PLAT_WINDOWS_SDK_VERSION >= BOOST_VERSION_NUMBER(0, 0, 9200)) || \
     (defined(__MINGW64__) && __MINGW64_VERSION_MAJOR >= 3))
#   undef BOOST_PLAT_WINDOWS_UWP
#   define BOOST_PLAT_WINDOWS_UWP BOOST_VERSION_NUMBER_AVAILABLE
#endif
#endif
#if BOOST_PLAT_WINDOWS_UWP
#   define BOOST_PLAT_WINDOWS_UWP_AVAILABLE
#   include <boost/predef/detail/platform_detected.h>
#   include <winapifamily.h>
#endif
#define BOOST_PLAT_WINDOWS_UWP_NAME "Universal Windows Platform"
#endif
#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_PLAT_WINDOWS_UWP, BOOST_PLAT_WINDOWS_UWP_NAME)
