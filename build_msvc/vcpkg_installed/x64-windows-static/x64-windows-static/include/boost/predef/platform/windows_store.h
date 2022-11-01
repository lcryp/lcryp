#ifndef BOOST_PREDEF_PLAT_WINDOWS_STORE_H
#define BOOST_PREDEF_PLAT_WINDOWS_STORE_H
#include <boost/predef/make.h>
#include <boost/predef/os/windows.h>
#include <boost/predef/platform/windows_uwp.h>
#include <boost/predef/version_number.h>
#define BOOST_PLAT_WINDOWS_STORE BOOST_VERSION_NUMBER_NOT_AVAILABLE
#if BOOST_OS_WINDOWS && \
    ((defined(WINAPI_FAMILY_PC_APP) && WINAPI_FAMILY == WINAPI_FAMILY_PC_APP) || \
     (defined(WINAPI_FAMILY_APP)    && WINAPI_FAMILY == WINAPI_FAMILY_APP))
#   undef BOOST_PLAT_WINDOWS_STORE
#   define BOOST_PLAT_WINDOWS_STORE BOOST_VERSION_NUMBER_AVAILABLE
#endif
#if BOOST_PLAT_WINDOWS_STORE
#   define BOOST_PLAT_WINDOWS_STORE_AVAILABLE
#   include <boost/predef/detail/platform_detected.h>
#endif
#define BOOST_PLAT_WINDOWS_STORE_NAME "Windows Store"
#endif
#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_PLAT_WINDOWS_STORE,BOOST_PLAT_WINDOWS_STORE_NAME)
