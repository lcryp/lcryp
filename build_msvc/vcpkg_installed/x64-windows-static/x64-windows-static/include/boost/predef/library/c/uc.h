#ifndef BOOST_PREDEF_LIBRARY_C_UC_H
#define BOOST_PREDEF_LIBRARY_C_UC_H
#include <boost/predef/library/c/_prefix.h>
#include <boost/predef/version_number.h>
#include <boost/predef/make.h>
#define BOOST_LIB_C_UC BOOST_VERSION_NUMBER_NOT_AVAILABLE
#if defined(__UCLIBC__)
#   undef BOOST_LIB_C_UC
#   define BOOST_LIB_C_UC BOOST_VERSION_NUMBER(\
        __UCLIBC_MAJOR__,__UCLIBC_MINOR__,__UCLIBC_SUBLEVEL__)
#endif
#if BOOST_LIB_C_UC
#   define BOOST_LIB_C_UC_AVAILABLE
#endif
#define BOOST_LIB_C_UC_NAME "uClibc"
#endif
#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_LIB_C_UC,BOOST_LIB_C_UC_NAME)
