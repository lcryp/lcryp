#ifndef BOOST_PREDEF_OS_HAIKU_H
#define BOOST_PREDEF_OS_HAIKU_H
#include <boost/predef/version_number.h>
#include <boost/predef/make.h>
#define BOOST_OS_HAIKU BOOST_VERSION_NUMBER_NOT_AVAILABLE
#if !defined(BOOST_PREDEF_DETAIL_OS_DETECTED) && ( \
    defined(__HAIKU__) \
    )
#   undef BOOST_OS_HAIKU
#   define BOOST_OS_HAIKU BOOST_VERSION_NUMBER_AVAILABLE
#endif
#if BOOST_OS_HAIKU
#   define BOOST_OS_HAIKU_AVAILABLE
#   include <boost/predef/detail/os_detected.h>
#endif
#define BOOST_OS_HAIKU_NAME "Haiku"
#endif
#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_OS_HAIKU,BOOST_OS_HAIKU_NAME)
