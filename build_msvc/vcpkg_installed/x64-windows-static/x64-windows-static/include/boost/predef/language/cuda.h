#ifndef BOOST_PREDEF_LANGUAGE_CUDA_H
#define BOOST_PREDEF_LANGUAGE_CUDA_H
#include <boost/predef/version_number.h>
#include <boost/predef/make.h>
#define BOOST_LANG_CUDA BOOST_VERSION_NUMBER_NOT_AVAILABLE
#if defined(__CUDACC__) || defined(__CUDA__)
#   undef BOOST_LANG_CUDA
#   include <cuda.h>
#   if defined(CUDA_VERSION)
#       define BOOST_LANG_CUDA BOOST_PREDEF_MAKE_10_VVRRP(CUDA_VERSION)
#   else
#       define BOOST_LANG_CUDA BOOST_VERSION_NUMBER_AVAILABLE
#   endif
#endif
#if BOOST_LANG_CUDA
#   define BOOST_LANG_CUDA_AVAILABLE
#endif
#define BOOST_LANG_CUDA_NAME "CUDA C/C++"
#endif
#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_LANG_CUDA,BOOST_LANG_CUDA_NAME)
