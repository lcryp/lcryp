#ifndef BOOST_PREDEF_LIBRARY_STD_SGI_H
#define BOOST_PREDEF_LIBRARY_STD_SGI_H
#include <boost/predef/library/std/_prefix.h>
#include <boost/predef/version_number.h>
#include <boost/predef/make.h>
#define BOOST_LIB_STD_SGI BOOST_VERSION_NUMBER_NOT_AVAILABLE
#if defined(__STL_CONFIG_H)
#   undef BOOST_LIB_STD_SGI
#   if defined(__SGI_STL)
#       define BOOST_LIB_STD_SGI BOOST_PREDEF_MAKE_0X_VRP(__SGI_STL)
#   else
#       define BOOST_LIB_STD_SGI BOOST_VERSION_NUMBER_AVAILABLE
#   endif
#endif
#if BOOST_LIB_STD_SGI
#   define BOOST_LIB_STD_SGI_AVAILABLE
#endif
#define BOOST_LIB_STD_SGI_NAME "SGI"
#endif
#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_LIB_STD_SGI,BOOST_LIB_STD_SGI_NAME)
