#ifndef B2_CONFIG_H
#define B2_CONFIG_H
#define OPT_HEADER_CACHE_EXT 1
#define OPT_GRAPH_DEBUG_EXT 1
#define OPT_SEMAPHORE 1
#define OPT_AT_FILES 1
#define OPT_DEBUG_PROFILE 1
#define JAM_DEBUGGER 1
#define OPT_FIX_TARGET_VARIABLES_EXT 1
#define OPT_IMPROVED_PATIENCE_EXT 1
#if defined(_WIN32) || defined(_WIN64) || \
    defined(__WIN32__) || defined(__TOS_WIN__) || \
    defined(__WINDOWS__)
    #define NT 1
#endif
#if defined(__VMS) || defined(__VMS_VER)
    #if !defined(VMS)
        #define VMS 1
    #endif
#endif
#if !defined(NT) && !defined(VMS)
#   define _FILE_OFFSET_BITS 64
#endif
#include <stdint.h>
#ifndef INT32_MIN
#if UINT_MAX == 0xffffffff
typedef int int32_t;
#elif (USHRT_MAX == 0xffffffff)
typedef short int32_t;
#elif ULONG_MAX == 0xffffffff
typedef long int32_t;
#endif
#endif
#endif
