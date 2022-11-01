#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif
#include <functional>
#ifndef __cpp_lib_not_fn
#error "Macro << __cpp_lib_not_fn is not set"
#endif
#if __cpp_lib_not_fn < 201603
#error "Macro __cpp_lib_not_fn had too low a value"
#endif
int main( int, char *[] )
{
   return 0;
}
