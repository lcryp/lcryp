#ifndef BOOST_PREDEF_OS_BSD_H
#define BOOST_PREDEF_OS_BSD_H
#include <boost/predef/os/macos.h>
#include <boost/predef/version_number.h>
#include <boost/predef/make.h>
#include <boost/predef/os/bsd/bsdi.h>
#include <boost/predef/os/bsd/dragonfly.h>
#include <boost/predef/os/bsd/free.h>
#include <boost/predef/os/bsd/open.h>
#include <boost/predef/os/bsd/net.h>
#ifndef BOOST_OS_BSD
#define BOOST_OS_BSD BOOST_VERSION_NUMBER_NOT_AVAILABLE
#endif
#if !defined(BOOST_PREDEF_DETAIL_OS_DETECTED) && ( \
    defined(BSD) || \
    defined(_SYSTYPE_BSD) \
    )
#   undef BOOST_OS_BSD
#   include <sys/param.h>
#   if !defined(BOOST_OS_BSD) && defined(BSD4_4)
#       define BOOST_OS_BSD BOOST_VERSION_NUMBER(4,4,0)
#   endif
#   if !defined(BOOST_OS_BSD) && defined(BSD4_3)
#       define BOOST_OS_BSD BOOST_VERSION_NUMBER(4,3,0)
#   endif
#   if !defined(BOOST_OS_BSD) && defined(BSD4_2)
#       define BOOST_OS_BSD BOOST_VERSION_NUMBER(4,2,0)
#   endif
#   if !defined(BOOST_OS_BSD) && defined(BSD)
#       define BOOST_OS_BSD BOOST_PREDEF_MAKE_10_VVRR(BSD)
#   endif
#   if !defined(BOOST_OS_BSD)
#       define BOOST_OS_BSD BOOST_VERSION_NUMBER_AVAILABLE
#   endif
#endif
#if BOOST_OS_BSD
#   define BOOST_OS_BSD_AVAILABLE
#   include <boost/predef/detail/os_detected.h>
#endif
#define BOOST_OS_BSD_NAME "BSD"
#endif
#include <boost/predef/os/bsd/bsdi.h>
#include <boost/predef/os/bsd/dragonfly.h>
#include <boost/predef/os/bsd/free.h>
#include <boost/predef/os/bsd/open.h>
#include <boost/predef/os/bsd/net.h>
#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_OS_BSD,BOOST_OS_BSD_NAME)
