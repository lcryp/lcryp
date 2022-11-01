#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif
#include <complex>
#ifndef __cpp_lib_complex_udls
#error "Macro << __cpp_lib_complex_udls is not set"
#endif
#if __cpp_lib_complex_udls < 201309
#error "Macro __cpp_lib_complex_udls had too low a value"
#endif
int main( int, char *[] )
{
   return 0;
}
