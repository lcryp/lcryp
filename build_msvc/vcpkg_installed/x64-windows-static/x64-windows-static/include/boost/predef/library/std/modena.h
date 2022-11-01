#ifndef BOOST_PREDEF_LIBRARY_STD_MODENA_H
#define BOOST_PREDEF_LIBRARY_STD_MODENA_H
#include <boost/predef/library/std/_prefix.h>
#include <boost/predef/version_number.h>
#include <boost/predef/make.h>
#define BOOST_LIB_STD_MSIPL BOOST_VERSION_NUMBER_NOT_AVAILABLE
#if defined(MSIPL_COMPILE_H) || defined(__MSIPL_COMPILE_H)
#   undef BOOST_LIB_STD_MSIPL
#   define BOOST_LIB_STD_MSIPL BOOST_VERSION_NUMBER_AVAILABLE
#endif
#if BOOST_LIB_STD_MSIPL
#   define BOOST_LIB_STD_MSIPL_AVAILABLE
#endif
#define BOOST_LIB_STD_MSIPL_NAME "Modena Software Lib++"
#endif
#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_LIB_STD_MSIPL,BOOST_LIB_STD_MSIPL_NAME)
