#ifndef BOOST_PREDEF_LIBRARY_STD_CXX_H
#define BOOST_PREDEF_LIBRARY_STD_CXX_H
#include <boost/predef/library/std/_prefix.h>
#include <boost/predef/version_number.h>
#include <boost/predef/make.h>
#define BOOST_LIB_STD_CXX BOOST_VERSION_NUMBER_NOT_AVAILABLE
#if defined(_LIBCPP_VERSION)
#   undef BOOST_LIB_STD_CXX
#   define BOOST_LIB_STD_CXX BOOST_PREDEF_MAKE_10_VVPPP(_LIBCPP_VERSION)
#endif
#if BOOST_LIB_STD_CXX
#   define BOOST_LIB_STD_CXX_AVAILABLE
#endif
#define BOOST_LIB_STD_CXX_NAME "libc++"
#endif
#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_LIB_STD_CXX,BOOST_LIB_STD_CXX_NAME)
