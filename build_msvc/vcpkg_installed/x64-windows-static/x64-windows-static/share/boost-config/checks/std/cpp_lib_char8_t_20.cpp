#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif
#include <atomic>
#ifndef __cpp_lib_char8_t
#error "Macro << __cpp_lib_char8_t is not set"
#endif
#if __cpp_lib_char8_t < 201811
#error "Macro __cpp_lib_char8_t had too low a value"
#endif
int main( int, char *[] )
{
   return 0;
}
