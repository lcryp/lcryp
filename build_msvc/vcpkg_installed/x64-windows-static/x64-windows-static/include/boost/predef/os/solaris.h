#ifndef BOOST_PREDEF_OS_SOLARIS_H
#define BOOST_PREDEF_OS_SOLARIS_H
#include <boost/predef/version_number.h>
#include <boost/predef/make.h>
#define BOOST_OS_SOLARIS BOOST_VERSION_NUMBER_NOT_AVAILABLE
#if !defined(BOOST_PREDEF_DETAIL_OS_DETECTED) && ( \
    defined(sun) || defined(__sun) \
    )
#   undef BOOST_OS_SOLARIS
#   define BOOST_OS_SOLARIS BOOST_VERSION_NUMBER_AVAILABLE
#endif
#if BOOST_OS_SOLARIS
#   define BOOST_OS_SOLARIS_AVAILABLE
#   include <boost/predef/detail/os_detected.h>
#endif
#define BOOST_OS_SOLARIS_NAME "Solaris"
#endif
#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_OS_SOLARIS,BOOST_OS_SOLARIS_NAME)
