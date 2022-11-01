#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif
#include <cstddef>
#ifndef __cpp_lib_byte
#error "Macro << __cpp_lib_byte is not set"
#endif
#if __cpp_lib_byte < 201603
#error "Macro __cpp_lib_byte had too low a value"
#endif
int main( int, char *[] )
{
   return 0;
}
