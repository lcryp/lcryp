#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif
#include <bit>
#ifndef __cpp_lib_bit_cast
#error "Macro << __cpp_lib_bit_cast is not set"
#endif
#if __cpp_lib_bit_cast < 201806
#error "Macro __cpp_lib_bit_cast had too low a value"
#endif
int main( int, char *[] )
{
   return 0;
}