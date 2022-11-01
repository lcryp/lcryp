#ifndef BOOST_PREDEF_LIBRARY_STD_VACPP_H
#define BOOST_PREDEF_LIBRARY_STD_VACPP_H
#include <boost/predef/library/std/_prefix.h>
#include <boost/predef/version_number.h>
#include <boost/predef/make.h>
#define BOOST_LIB_STD_IBM BOOST_VERSION_NUMBER_NOT_AVAILABLE
#if defined(__IBMCPP__)
#   undef BOOST_LIB_STD_IBM
#   define BOOST_LIB_STD_IBM BOOST_VERSION_NUMBER_AVAILABLE
#endif
#if BOOST_LIB_STD_IBM
#   define BOOST_LIB_STD_IBM_AVAILABLE
#endif
#define BOOST_LIB_STD_IBM_NAME "IBM VACPP"
#endif
#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_LIB_STD_IBM,BOOST_LIB_STD_IBM_NAME)
