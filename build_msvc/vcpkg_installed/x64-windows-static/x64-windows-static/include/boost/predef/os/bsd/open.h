#ifndef BOOST_PREDEF_OS_BSD_OPEN_H
#define BOOST_PREDEF_OS_BSD_OPEN_H
#include <boost/predef/os/bsd.h>
#define BOOST_OS_BSD_OPEN BOOST_VERSION_NUMBER_NOT_AVAILABLE
#if !defined(BOOST_PREDEF_DETAIL_OS_DETECTED) && ( \
    defined(__OpenBSD__) \
    )
#   ifndef BOOST_OS_BSD_AVAILABLE
#       undef BOOST_OS_BSD
#       define BOOST_OS_BSD BOOST_VERSION_NUMBER_AVAILABLE
#       define BOOST_OS_BSD_AVAILABLE
#   endif
#   undef BOOST_OS_BSD_OPEN
#   if !defined(BOOST_OS_BSD_OPEN) && defined(OpenBSD2_0)
#       define BOOST_OS_BSD_OPEN BOOST_VERSION_NUMBER(2,0,0)
#   endif
#   if !defined(BOOST_OS_BSD_OPEN) && defined(OpenBSD2_1)
#       define BOOST_OS_BSD_OPEN BOOST_VERSION_NUMBER(2,1,0)
#   endif
#   if !defined(BOOST_OS_BSD_OPEN) && defined(OpenBSD2_2)
#       define BOOST_OS_BSD_OPEN BOOST_VERSION_NUMBER(2,2,0)
#   endif
#   if !defined(BOOST_OS_BSD_OPEN) && defined(OpenBSD2_3)
#       define BOOST_OS_BSD_OPEN BOOST_VERSION_NUMBER(2,3,0)
#   endif
#   if !defined(BOOST_OS_BSD_OPEN) && defined(OpenBSD2_4)
#       define BOOST_OS_BSD_OPEN BOOST_VERSION_NUMBER(2,4,0)
#   endif
#   if !defined(BOOST_OS_BSD_OPEN) && defined(OpenBSD2_5)
#       define BOOST_OS_BSD_OPEN BOOST_VERSION_NUMBER(2,5,0)
#   endif
#   if !defined(BOOST_OS_BSD_OPEN) && defined(OpenBSD2_6)
#       define BOOST_OS_BSD_OPEN BOOST_VERSION_NUMBER(2,6,0)
#   endif
#   if !defined(BOOST_OS_BSD_OPEN) && defined(OpenBSD2_7)
#       define BOOST_OS_BSD_OPEN BOOST_VERSION_NUMBER(2,7,0)
#   endif
#   if !defined(BOOST_OS_BSD_OPEN) && defined(OpenBSD2_8)
#       define BOOST_OS_BSD_OPEN BOOST_VERSION_NUMBER(2,8,0)
#   endif
#   if !defined(BOOST_OS_BSD_OPEN) && defined(OpenBSD2_9)
#       define BOOST_OS_BSD_OPEN BOOST_VERSION_NUMBER(2,9,0)
#   endif
#   if !defined(BOOST_OS_BSD_OPEN) && defined(OpenBSD3_0)
#       define BOOST_OS_BSD_OPEN BOOST_VERSION_NUMBER(3,0,0)
#   endif
#   if !defined(BOOST_OS_BSD_OPEN) && defined(OpenBSD3_1)
#       define BOOST_OS_BSD_OPEN BOOST_VERSION_NUMBER(3,1,0)
#   endif
#   if !defined(BOOST_OS_BSD_OPEN) && defined(OpenBSD3_2)
#       define BOOST_OS_BSD_OPEN BOOST_VERSION_NUMBER(3,2,0)
#   endif
#   if !defined(BOOST_OS_BSD_OPEN) && defined(OpenBSD3_3)
#       define BOOST_OS_BSD_OPEN BOOST_VERSION_NUMBER(3,3,0)
#   endif
#   if !defined(BOOST_OS_BSD_OPEN) && defined(OpenBSD3_4)
#       define BOOST_OS_BSD_OPEN BOOST_VERSION_NUMBER(3,4,0)
#   endif
#   if !defined(BOOST_OS_BSD_OPEN) && defined(OpenBSD3_5)
#       define BOOST_OS_BSD_OPEN BOOST_VERSION_NUMBER(3,5,0)
#   endif
#   if !defined(BOOST_OS_BSD_OPEN) && defined(OpenBSD3_6)
#       define BOOST_OS_BSD_OPEN BOOST_VERSION_NUMBER(3,6,0)
#   endif
#   if !defined(BOOST_OS_BSD_OPEN) && defined(OpenBSD3_7)
#       define BOOST_OS_BSD_OPEN BOOST_VERSION_NUMBER(3,7,0)
#   endif
#   if !defined(BOOST_OS_BSD_OPEN) && defined(OpenBSD3_8)
#       define BOOST_OS_BSD_OPEN BOOST_VERSION_NUMBER(3,8,0)
#   endif
#   if !defined(BOOST_OS_BSD_OPEN) && defined(OpenBSD3_9)
#       define BOOST_OS_BSD_OPEN BOOST_VERSION_NUMBER(3,9,0)
#   endif
#   if !defined(BOOST_OS_BSD_OPEN) && defined(OpenBSD4_0)
#       define BOOST_OS_BSD_OPEN BOOST_VERSION_NUMBER(4,0,0)
#   endif
#   if !defined(BOOST_OS_BSD_OPEN) && defined(OpenBSD4_1)
#       define BOOST_OS_BSD_OPEN BOOST_VERSION_NUMBER(4,1,0)
#   endif
#   if !defined(BOOST_OS_BSD_OPEN) && defined(OpenBSD4_2)
#       define BOOST_OS_BSD_OPEN BOOST_VERSION_NUMBER(4,2,0)
#   endif
#   if !defined(BOOST_OS_BSD_OPEN) && defined(OpenBSD4_3)
#       define BOOST_OS_BSD_OPEN BOOST_VERSION_NUMBER(4,3,0)
#   endif
#   if !defined(BOOST_OS_BSD_OPEN) && defined(OpenBSD4_4)
#       define BOOST_OS_BSD_OPEN BOOST_VERSION_NUMBER(4,4,0)
#   endif
#   if !defined(BOOST_OS_BSD_OPEN) && defined(OpenBSD4_5)
#       define BOOST_OS_BSD_OPEN BOOST_VERSION_NUMBER(4,5,0)
#   endif
#   if !defined(BOOST_OS_BSD_OPEN) && defined(OpenBSD4_6)
#       define BOOST_OS_BSD_OPEN BOOST_VERSION_NUMBER(4,6,0)
#   endif
#   if !defined(BOOST_OS_BSD_OPEN) && defined(OpenBSD4_7)
#       define BOOST_OS_BSD_OPEN BOOST_VERSION_NUMBER(4,7,0)
#   endif
#   if !defined(BOOST_OS_BSD_OPEN) && defined(OpenBSD4_8)
#       define BOOST_OS_BSD_OPEN BOOST_VERSION_NUMBER(4,8,0)
#   endif
#   if !defined(BOOST_OS_BSD_OPEN) && defined(OpenBSD4_9)
#       define BOOST_OS_BSD_OPEN BOOST_VERSION_NUMBER(4,9,0)
#   endif
#   if !defined(BOOST_OS_BSD_OPEN) && defined(OpenBSD5_0)
#       define BOOST_OS_BSD_OPEN BOOST_VERSION_NUMBER(5,0,0)
#   endif
#   if !defined(BOOST_OS_BSD_OPEN) && defined(OpenBSD5_1)
#       define BOOST_OS_BSD_OPEN BOOST_VERSION_NUMBER(5,1,0)
#   endif
#   if !defined(BOOST_OS_BSD_OPEN) && defined(OpenBSD5_2)
#       define BOOST_OS_BSD_OPEN BOOST_VERSION_NUMBER(5,2,0)
#   endif
#   if !defined(BOOST_OS_BSD_OPEN) && defined(OpenBSD5_3)
#       define BOOST_OS_BSD_OPEN BOOST_VERSION_NUMBER(5,3,0)
#   endif
#   if !defined(BOOST_OS_BSD_OPEN) && defined(OpenBSD5_4)
#       define BOOST_OS_BSD_OPEN BOOST_VERSION_NUMBER(5,4,0)
#   endif
#   if !defined(BOOST_OS_BSD_OPEN) && defined(OpenBSD5_5)
#       define BOOST_OS_BSD_OPEN BOOST_VERSION_NUMBER(5,5,0)
#   endif
#   if !defined(BOOST_OS_BSD_OPEN) && defined(OpenBSD5_6)
#       define BOOST_OS_BSD_OPEN BOOST_VERSION_NUMBER(5,6,0)
#   endif
#   if !defined(BOOST_OS_BSD_OPEN) && defined(OpenBSD5_7)
#       define BOOST_OS_BSD_OPEN BOOST_VERSION_NUMBER(5,7,0)
#   endif
#   if !defined(BOOST_OS_BSD_OPEN) && defined(OpenBSD5_8)
#       define BOOST_OS_BSD_OPEN BOOST_VERSION_NUMBER(5,8,0)
#   endif
#   if !defined(BOOST_OS_BSD_OPEN) && defined(OpenBSD5_9)
#       define BOOST_OS_BSD_OPEN BOOST_VERSION_NUMBER(5,9,0)
#   endif
#   if !defined(BOOST_OS_BSD_OPEN) && defined(OpenBSD6_0)
#       define BOOST_OS_BSD_OPEN BOOST_VERSION_NUMBER(6,0,0)
#   endif
#   if !defined(BOOST_OS_BSD_OPEN) && defined(OpenBSD6_1)
#       define BOOST_OS_BSD_OPEN BOOST_VERSION_NUMBER(6,1,0)
#   endif
#   if !defined(BOOST_OS_BSD_OPEN) && defined(OpenBSD6_2)
#       define BOOST_OS_BSD_OPEN BOOST_VERSION_NUMBER(6,2,0)
#   endif
#   if !defined(BOOST_OS_BSD_OPEN) && defined(OpenBSD6_3)
#       define BOOST_OS_BSD_OPEN BOOST_VERSION_NUMBER(6,3,0)
#   endif
#   if !defined(BOOST_OS_BSD_OPEN) && defined(OpenBSD6_4)
#       define BOOST_OS_BSD_OPEN BOOST_VERSION_NUMBER(6,4,0)
#   endif
#   if !defined(BOOST_OS_BSD_OPEN) && defined(OpenBSD6_5)
#       define BOOST_OS_BSD_OPEN BOOST_VERSION_NUMBER(6,5,0)
#   endif
#   if !defined(BOOST_OS_BSD_OPEN) && defined(OpenBSD6_6)
#       define BOOST_OS_BSD_OPEN BOOST_VERSION_NUMBER(6,6,0)
#   endif
#   if !defined(BOOST_OS_BSD_OPEN) && defined(OpenBSD6_7)
#       define BOOST_OS_BSD_OPEN BOOST_VERSION_NUMBER(6,7,0)
#   endif
#   if !defined(BOOST_OS_BSD_OPEN) && defined(OpenBSD6_8)
#       define BOOST_OS_BSD_OPEN BOOST_VERSION_NUMBER(6,8,0)
#   endif
#   if !defined(BOOST_OS_BSD_OPEN) && defined(OpenBSD6_9)
#       define BOOST_OS_BSD_OPEN BOOST_VERSION_NUMBER(6,9,0)
#   endif
#   if !defined(BOOST_OS_BSD_OPEN)
#       define BOOST_OS_BSD_OPEN BOOST_VERSION_NUMBER_AVAILABLE
#   endif
#endif
#if BOOST_OS_BSD_OPEN
#   define BOOST_OS_BSD_OPEN_AVAILABLE
#   include <boost/predef/detail/os_detected.h>
#endif
#define BOOST_OS_BSD_OPEN_NAME "OpenBSD"
#endif
#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_OS_BSD_OPEN,BOOST_OS_BSD_OPEN_NAME)
