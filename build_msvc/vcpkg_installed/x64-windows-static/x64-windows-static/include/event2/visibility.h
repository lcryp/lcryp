#ifndef EVENT2_VISIBILITY_H_INCLUDED_
#define EVENT2_VISIBILITY_H_INCLUDED_
#include <event2/event-config.h>
#if defined(event_shared_EXPORTS) || \
    defined(event_extra_shared_EXPORTS) || \
    defined(event_core_shared_EXPORTS) || \
    defined(event_pthreads_shared_EXPORTS) || \
    defined(event_openssl_shared_EXPORTS)
# if defined (__SUNPRO_C) && (__SUNPRO_C >= 0x550)
#  define EVENT2_EXPORT_SYMBOL __global
# elif defined __GNUC__
#  define EVENT2_EXPORT_SYMBOL __attribute__ ((visibility("default")))
# elif defined(_MSC_VER)
#  define EVENT2_EXPORT_SYMBOL __declspec(dllexport)
# else
#  define EVENT2_EXPORT_SYMBOL
# endif
#else
# define EVENT2_EXPORT_SYMBOL
#endif
#if defined(_MSC_VER)
# if defined(event_core_shared_EXPORTS)
#  define EVENT2_CORE_EXPORT_SYMBOL __declspec(dllexport)
# elif defined(event_extra_shared_EXPORTS) ||   \
       defined(EVENT_VISIBILITY_WANT_DLLIMPORT)
#  define EVENT2_CORE_EXPORT_SYMBOL __declspec(dllimport)
# endif
#endif
#if !defined(EVENT2_CORE_EXPORT_SYMBOL)
# define EVENT2_CORE_EXPORT_SYMBOL EVENT2_EXPORT_SYMBOL
#endif
#endif