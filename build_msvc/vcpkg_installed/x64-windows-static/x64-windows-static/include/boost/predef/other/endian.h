#ifndef BOOST_PREDEF_ENDIAN_H
#define BOOST_PREDEF_ENDIAN_H
#include <boost/predef/version_number.h>
#include <boost/predef/make.h>
#include <boost/predef/library/c/gnu.h>
#include <boost/predef/os/macos.h>
#include <boost/predef/os/bsd.h>
#include <boost/predef/platform/android.h>
#define BOOST_ENDIAN_BIG_BYTE BOOST_VERSION_NUMBER_NOT_AVAILABLE
#define BOOST_ENDIAN_BIG_WORD BOOST_VERSION_NUMBER_NOT_AVAILABLE
#define BOOST_ENDIAN_LITTLE_BYTE BOOST_VERSION_NUMBER_NOT_AVAILABLE
#define BOOST_ENDIAN_LITTLE_WORD BOOST_VERSION_NUMBER_NOT_AVAILABLE
#if !BOOST_ENDIAN_BIG_BYTE && !BOOST_ENDIAN_BIG_WORD && \
    !BOOST_ENDIAN_LITTLE_BYTE && !BOOST_ENDIAN_LITTLE_WORD
#   if BOOST_LIB_C_GNU || BOOST_PLAT_ANDROID || BOOST_OS_BSD_OPEN
#       include <endian.h>
#   else
#       if BOOST_OS_MACOS
#           include <machine/endian.h>
#       else
#           if BOOST_OS_BSD
#               include <sys/endian.h>
#           endif
#       endif
#   endif
#   if defined(__BYTE_ORDER)
#       if defined(__BIG_ENDIAN) && (__BYTE_ORDER == __BIG_ENDIAN)
#           undef BOOST_ENDIAN_BIG_BYTE
#           define BOOST_ENDIAN_BIG_BYTE BOOST_VERSION_NUMBER_AVAILABLE
#       endif
#       if defined(__LITTLE_ENDIAN) && (__BYTE_ORDER == __LITTLE_ENDIAN)
#           undef BOOST_ENDIAN_LITTLE_BYTE
#           define BOOST_ENDIAN_LITTLE_BYTE BOOST_VERSION_NUMBER_AVAILABLE
#       endif
#       if defined(__PDP_ENDIAN) && (__BYTE_ORDER == __PDP_ENDIAN)
#           undef BOOST_ENDIAN_LITTLE_WORD
#           define BOOST_ENDIAN_LITTLE_WORD BOOST_VERSION_NUMBER_AVAILABLE
#       endif
#   endif
#   if !defined(__BYTE_ORDER) && defined(_BYTE_ORDER)
#       if defined(_BIG_ENDIAN) && (_BYTE_ORDER == _BIG_ENDIAN)
#           undef BOOST_ENDIAN_BIG_BYTE
#           define BOOST_ENDIAN_BIG_BYTE BOOST_VERSION_NUMBER_AVAILABLE
#       endif
#       if defined(_LITTLE_ENDIAN) && (_BYTE_ORDER == _LITTLE_ENDIAN)
#           undef BOOST_ENDIAN_LITTLE_BYTE
#           define BOOST_ENDIAN_LITTLE_BYTE BOOST_VERSION_NUMBER_AVAILABLE
#       endif
#       if defined(_PDP_ENDIAN) && (_BYTE_ORDER == _PDP_ENDIAN)
#           undef BOOST_ENDIAN_LITTLE_WORD
#           define BOOST_ENDIAN_LITTLE_WORD BOOST_VERSION_NUMBER_AVAILABLE
#       endif
#   endif
#endif
#if !BOOST_ENDIAN_BIG_BYTE && !BOOST_ENDIAN_BIG_WORD && \
    !BOOST_ENDIAN_LITTLE_BYTE && !BOOST_ENDIAN_LITTLE_WORD
#   if (defined(__BIG_ENDIAN__) && !defined(__LITTLE_ENDIAN__)) || \
       (defined(_BIG_ENDIAN) && !defined(_LITTLE_ENDIAN)) || \
        defined(__ARMEB__) || \
        defined(__THUMBEB__) || \
        defined(__AARCH64EB__) || \
        defined(_MIPSEB) || \
        defined(__MIPSEB) || \
        defined(__MIPSEB__)
#       undef BOOST_ENDIAN_BIG_BYTE
#       define BOOST_ENDIAN_BIG_BYTE BOOST_VERSION_NUMBER_AVAILABLE
#   endif
#endif
#if !BOOST_ENDIAN_BIG_BYTE && !BOOST_ENDIAN_BIG_WORD && \
    !BOOST_ENDIAN_LITTLE_BYTE && !BOOST_ENDIAN_LITTLE_WORD
#   if (defined(__LITTLE_ENDIAN__) && !defined(__BIG_ENDIAN__)) || \
       (defined(_LITTLE_ENDIAN) && !defined(_BIG_ENDIAN)) || \
        defined(__ARMEL__) || \
        defined(__THUMBEL__) || \
        defined(__AARCH64EL__) || \
        defined(__loongarch__) || \
        defined(_MIPSEL) || \
        defined(__MIPSEL) || \
        defined(__MIPSEL__) || \
        defined(__riscv) || \
        defined(__e2k__)
#       undef BOOST_ENDIAN_LITTLE_BYTE
#       define BOOST_ENDIAN_LITTLE_BYTE BOOST_VERSION_NUMBER_AVAILABLE
#   endif
#endif
#if !BOOST_ENDIAN_BIG_BYTE && !BOOST_ENDIAN_BIG_WORD && \
    !BOOST_ENDIAN_LITTLE_BYTE && !BOOST_ENDIAN_LITTLE_WORD
#   include <boost/predef/architecture.h>
#   if BOOST_ARCH_M68K || \
        BOOST_ARCH_PARISC || \
        BOOST_ARCH_SPARC || \
        BOOST_ARCH_SYS370 || \
        BOOST_ARCH_SYS390 || \
        BOOST_ARCH_Z
#       undef BOOST_ENDIAN_BIG_BYTE
#       define BOOST_ENDIAN_BIG_BYTE BOOST_VERSION_NUMBER_AVAILABLE
#   endif
#   if BOOST_ARCH_IA64 || \
        BOOST_ARCH_X86 || \
        BOOST_ARCH_BLACKFIN
#       undef BOOST_ENDIAN_LITTLE_BYTE
#       define BOOST_ENDIAN_LITTLE_BYTE BOOST_VERSION_NUMBER_AVAILABLE
#   endif
#endif
#if !BOOST_ENDIAN_BIG_BYTE && !BOOST_ENDIAN_BIG_WORD && \
    !BOOST_ENDIAN_LITTLE_BYTE && !BOOST_ENDIAN_LITTLE_WORD
#   if BOOST_ARCH_ARM
#       include <boost/predef/os/windows.h>
#       if BOOST_OS_WINDOWS
#           undef BOOST_ENDIAN_LITTLE_BYTE
#           define BOOST_ENDIAN_LITTLE_BYTE BOOST_VERSION_NUMBER_AVAILABLE
#       endif
#   endif
#endif
#if BOOST_ENDIAN_BIG_BYTE
#   define BOOST_ENDIAN_BIG_BYTE_AVAILABLE
#endif
#if BOOST_ENDIAN_BIG_WORD
#   define BOOST_ENDIAN_BIG_WORD_BYTE_AVAILABLE
#endif
#if BOOST_ENDIAN_LITTLE_BYTE
#   define BOOST_ENDIAN_LITTLE_BYTE_AVAILABLE
#endif
#if BOOST_ENDIAN_LITTLE_WORD
#   define BOOST_ENDIAN_LITTLE_WORD_BYTE_AVAILABLE
#endif
#define BOOST_ENDIAN_BIG_BYTE_NAME "Byte-Swapped Big-Endian"
#define BOOST_ENDIAN_BIG_WORD_NAME "Word-Swapped Big-Endian"
#define BOOST_ENDIAN_LITTLE_BYTE_NAME "Byte-Swapped Little-Endian"
#define BOOST_ENDIAN_LITTLE_WORD_NAME "Word-Swapped Little-Endian"
#endif
#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_ENDIAN_BIG_BYTE,BOOST_ENDIAN_BIG_BYTE_NAME)
#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_ENDIAN_BIG_WORD,BOOST_ENDIAN_BIG_WORD_NAME)
#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_ENDIAN_LITTLE_BYTE,BOOST_ENDIAN_LITTLE_BYTE_NAME)
#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_ENDIAN_LITTLE_WORD,BOOST_ENDIAN_LITTLE_WORD_NAME)
