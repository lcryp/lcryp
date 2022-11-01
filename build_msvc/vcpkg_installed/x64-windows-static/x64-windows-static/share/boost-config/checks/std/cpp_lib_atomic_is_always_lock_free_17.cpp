#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif
#include <atomic>
#ifndef __cpp_lib_atomic_is_always_lock_free
#error "Macro << __cpp_lib_atomic_is_always_lock_free is not set"
#endif
#if __cpp_lib_atomic_is_always_lock_free < 201603
#error "Macro __cpp_lib_atomic_is_always_lock_free had too low a value"
#endif
int main( int, char *[] )
{
   return 0;
}
