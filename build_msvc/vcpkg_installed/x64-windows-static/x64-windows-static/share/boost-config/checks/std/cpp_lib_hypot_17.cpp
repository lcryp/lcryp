#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif
#include <cmath>
#ifndef __cpp_lib_hypot
#error "Macro << __cpp_lib_hypot is not set"
#endif
#if __cpp_lib_hypot < 201603
#error "Macro __cpp_lib_hypot had too low a value"
#endif
int main( int, char *[] )
{
   return 0;
}
