#ifndef BOOST_PREDEF_OS_CYGWIN_H
#define BOOST_PREDEF_OS_CYGWIN_H
#include <boost/predef/version_number.h>
#include <boost/predef/make.h>
#define BOOST_OS_CYGWIN BOOST_VERSION_NUMBER_NOT_AVAILABLE
#if !defined(BOOST_PREDEF_DETAIL_OS_DETECTED) && ( \
    defined(__CYGWIN__) \
    )
#   include <cygwin/version.h>
#   undef BOOST_OS_CYGWIN
#   define BOOST_OS_CYGWIN \
        BOOST_VERSION_NUMBER(CYGWIN_VERSION_API_MAJOR,\
                             CYGWIN_VERSION_API_MINOR, 0)
#endif
#if BOOST_OS_CYGWIN
#   define BOOST_OS_CYGWIN_AVAILABLE
#   include <boost/predef/detail/os_detected.h>
#endif
#define BOOST_OS_CYGWIN_NAME "Cygwin"
#endif
#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_OS_CYGWIN,BOOST_OS_CYGWIN_NAME)
