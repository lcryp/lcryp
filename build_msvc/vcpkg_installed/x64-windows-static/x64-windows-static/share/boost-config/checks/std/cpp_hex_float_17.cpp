#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif
#ifndef __cpp_hex_float
#error "Macro << __cpp_hex_float is not set"
#endif
#if __cpp_hex_float < 201603
#error "Macro __cpp_hex_float had too low a value"
#endif
int main( int, char *[] )
{
   return 0;
}
