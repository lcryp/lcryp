#include <boost/predef/architecture/x86/32.h>
#include <boost/predef/architecture/x86/64.h>
#ifndef BOOST_PREDEF_ARCHITECTURE_X86_H
#define BOOST_PREDEF_ARCHITECTURE_X86_H
#define BOOST_ARCH_X86 BOOST_VERSION_NUMBER_NOT_AVAILABLE
#if BOOST_ARCH_X86_32 || BOOST_ARCH_X86_64
#   undef BOOST_ARCH_X86
#   define BOOST_ARCH_X86 BOOST_VERSION_NUMBER_AVAILABLE
#endif
#if BOOST_ARCH_X86
#   define BOOST_ARCH_X86_AVAILABLE
#endif
#define BOOST_ARCH_X86_NAME "Intel x86"
#endif
#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_ARCH_X86,BOOST_ARCH_X86_NAME)
