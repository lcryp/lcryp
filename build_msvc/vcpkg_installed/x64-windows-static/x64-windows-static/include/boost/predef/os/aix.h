#ifndef BOOST_PREDEF_OS_AIX_H
#define BOOST_PREDEF_OS_AIX_H
#include <boost/predef/version_number.h>
#include <boost/predef/make.h>
#define BOOST_OS_AIX BOOST_VERSION_NUMBER_NOT_AVAILABLE
#if !defined(BOOST_PREDEF_DETAIL_OS_DETECTED) && ( \
    defined(_AIX) || defined(__TOS_AIX__) \
    )
#   undef BOOST_OS_AIX
#   if !defined(BOOST_OS_AIX) && defined(_AIX43)
#       define BOOST_OS_AIX BOOST_VERSION_NUMBER(4,3,0)
#   endif
#   if !defined(BOOST_OS_AIX) && defined(_AIX41)
#       define BOOST_OS_AIX BOOST_VERSION_NUMBER(4,1,0)
#   endif
#   if !defined(BOOST_OS_AIX) && defined(_AIX32)
#       define BOOST_OS_AIX BOOST_VERSION_NUMBER(3,2,0)
#   endif
#   if !defined(BOOST_OS_AIX) && defined(_AIX3)
#       define BOOST_OS_AIX BOOST_VERSION_NUMBER(3,0,0)
#   endif
#   if !defined(BOOST_OS_AIX)
#       define BOOST_OS_AIX BOOST_VERSION_NUMBER_AVAILABLE
#   endif
#endif
#if BOOST_OS_AIX
#   define BOOST_OS_AIX_AVAILABLE
#   include <boost/predef/detail/os_detected.h>
#endif
#define BOOST_OS_AIX_NAME "IBM AIX"
#endif
#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_OS_AIX,BOOST_OS_AIX_NAME)
