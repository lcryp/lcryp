#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif
#include <string>
#ifndef __cpp_lib_erase_if
#error "Macro << __cpp_lib_erase_if is not set"
#endif
#if __cpp_lib_erase_if < 201811
#error "Macro __cpp_lib_erase_if had too low a value"
#endif
int main( int, char *[] )
{
   return 0;
}
