#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif
#include <shared_mutex>
#ifndef __cpp_lib_shared_timed_mutex
#error "Macro << __cpp_lib_shared_timed_mutex is not set"
#endif
#if __cpp_lib_shared_timed_mutex < 201402
#error "Macro __cpp_lib_shared_timed_mutex had too low a value"
#endif
int main( int, char *[] )
{
   return 0;
}
