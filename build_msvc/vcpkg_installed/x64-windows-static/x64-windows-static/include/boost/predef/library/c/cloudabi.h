#ifndef BOOST_PREDEF_LIBRARY_C_CLOUDABI_H
#define BOOST_PREDEF_LIBRARY_C_CLOUDABI_H
#include <boost/predef/version_number.h>
#include <boost/predef/make.h>
#include <boost/predef/library/c/_prefix.h>
#if defined(__CloudABI__)
#include <stddef.h>
#endif
#define BOOST_LIB_C_CLOUDABI BOOST_VERSION_NUMBER_NOT_AVAILABLE
#if defined(__cloudlibc__)
#   undef BOOST_LIB_C_CLOUDABI
#   define BOOST_LIB_C_CLOUDABI \
            BOOST_VERSION_NUMBER(__cloudlibc_major__,__cloudlibc_minor__,0)
#endif
#if BOOST_LIB_C_CLOUDABI
#   define BOOST_LIB_C_CLOUDABI_AVAILABLE
#endif
#define BOOST_LIB_C_CLOUDABI_NAME "cloudlibc"
#endif
#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_LIB_C_CLOUDABI,BOOST_LIB_C_CLOUDABI_NAME)
