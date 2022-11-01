#ifndef BOOST_PREDEF_COMPILER_NVCC_H
#define BOOST_PREDEF_COMPILER_NVCC_H
#include <boost/predef/version_number.h>
#include <boost/predef/make.h>
#define BOOST_COMP_NVCC BOOST_VERSION_NUMBER_NOT_AVAILABLE
#if defined(__NVCC__)
#   if !defined(__CUDACC_VER_MAJOR__) || !defined(__CUDACC_VER_MINOR__) || !defined(__CUDACC_VER_BUILD__)
#       define BOOST_COMP_NVCC_DETECTION BOOST_VERSION_NUMBER_AVAILABLE
#   else
#       define BOOST_COMP_NVCC_DETECTION BOOST_VERSION_NUMBER(__CUDACC_VER_MAJOR__, __CUDACC_VER_MINOR__, __CUDACC_VER_BUILD__)
#   endif
#endif
#ifdef BOOST_COMP_NVCC_DETECTION
#   undef BOOST_COMP_NVCC
#   define BOOST_COMP_NVCC BOOST_COMP_NVCC_DETECTION
#   define BOOST_COMP_NVCC_AVAILABLE
#   include <boost/predef/detail/comp_detected.h>
#endif
#define BOOST_COMP_NVCC_NAME "NVCC"
#endif
#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_COMP_NVCC,BOOST_COMP_NVCC_NAME)
