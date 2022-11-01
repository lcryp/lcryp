#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif
#include <atomic>
#ifndef __cpp_lib_atomic_ref
#error "Macro << __cpp_lib_atomic_ref is not set"
#endif
#if __cpp_lib_atomic_ref < 201806
#error "Macro __cpp_lib_atomic_ref had too low a value"
#endif
int main( int, char *[] )
{
   return 0;
}
