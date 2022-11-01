#ifndef BOOST_PREDEF_PLAT_CLOUDABI_H
#define BOOST_PREDEF_PLAT_CLOUDABI_H
#include <boost/predef/version_number.h>
#include <boost/predef/make.h>
#define BOOST_PLAT_CLOUDABI BOOST_VERSION_NUMBER_NOT_AVAILABLE
#if defined(__CloudABI__)
#   undef BOOST_PLAT_CLOUDABI
#   define BOOST_PLAT_CLOUDABI BOOST_VERSION_NUMBER_AVAILABLE
#endif
#if BOOST_PLAT_CLOUDABI
#   define BOOST_PLAT_CLOUDABI_AVAILABLE
#   include <boost/predef/detail/platform_detected.h>
#endif
#define BOOST_PLAT_CLOUDABI_NAME "CloudABI"
#endif
#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_PLAT_CLOUDABI,BOOST_PLAT_CLOUDABI_NAME)
