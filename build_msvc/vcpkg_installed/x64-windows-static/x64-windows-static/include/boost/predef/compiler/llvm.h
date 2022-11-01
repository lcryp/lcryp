#ifndef BOOST_PREDEF_COMPILER_LLVM_H
#define BOOST_PREDEF_COMPILER_LLVM_H
#include <boost/predef/compiler/clang.h>
#include <boost/predef/version_number.h>
#include <boost/predef/make.h>
#define BOOST_COMP_LLVM BOOST_VERSION_NUMBER_NOT_AVAILABLE
#if defined(__llvm__)
#   define BOOST_COMP_LLVM_DETECTION BOOST_VERSION_NUMBER_AVAILABLE
#endif
#ifdef BOOST_COMP_LLVM_DETECTION
#   if defined(BOOST_PREDEF_DETAIL_COMP_DETECTED)
#       define BOOST_COMP_LLVM_EMULATED BOOST_COMP_LLVM_DETECTION
#   else
#       undef BOOST_COMP_LLVM
#       define BOOST_COMP_LLVM BOOST_COMP_LLVM_DETECTION
#   endif
#   define BOOST_COMP_LLVM_AVAILABLE
#   include <boost/predef/detail/comp_detected.h>
#endif
#define BOOST_COMP_LLVM_NAME "LLVM"
#endif
#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_COMP_LLVM,BOOST_COMP_LLVM_NAME)
#ifdef BOOST_COMP_LLVM_EMULATED
#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_COMP_LLVM_EMULATED,BOOST_COMP_LLVM_NAME)
#endif
