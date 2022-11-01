#ifndef BOOST_PREDEF_OS_BEOS_H
#define BOOST_PREDEF_OS_BEOS_H
#include <boost/predef/version_number.h>
#include <boost/predef/make.h>
#define BOOST_OS_BEOS BOOST_VERSION_NUMBER_NOT_AVAILABLE
#if !defined(BOOST_PREDEF_DETAIL_OS_DETECTED) && ( \
    defined(__BEOS__) \
    )
#   undef BOOST_OS_BEOS
#   define BOOST_OS_BEOS BOOST_VERSION_NUMBER_AVAILABLE
#endif
#if BOOST_OS_BEOS
#   define BOOST_OS_BEOS_AVAILABLE
#   include <boost/predef/detail/os_detected.h>
#endif
#define BOOST_OS_BEOS_NAME "BeOS"
#endif
#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_OS_BEOS,BOOST_OS_BEOS_NAME)
