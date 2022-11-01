#ifndef BOOST_PREDEF_LIBRARY_STD_LIBCOMO_H
#define BOOST_PREDEF_LIBRARY_STD_LIBCOMO_H
#include <boost/predef/library/std/_prefix.h>
#include <boost/predef/version_number.h>
#include <boost/predef/make.h>
#define BOOST_LIB_STD_COMO BOOST_VERSION_NUMBER_NOT_AVAILABLE
#if defined(__LIBCOMO__)
#   undef BOOST_LIB_STD_COMO
#   define BOOST_LIB_STD_COMO BOOST_VERSION_NUMBER(__LIBCOMO_VERSION__,0,0)
#endif
#if BOOST_LIB_STD_COMO
#   define BOOST_LIB_STD_COMO_AVAILABLE
#endif
#define BOOST_LIB_STD_COMO_NAME "Comeau Computing"
#endif
#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_LIB_STD_COMO,BOOST_LIB_STD_COMO_NAME)
