#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif
#include <memory>
#ifndef __cpp_lib_transparent_operators
#error "Macro << __cpp_lib_transparent_operators is not set"
#endif
#if __cpp_lib_transparent_operators < 201510
#error "Macro __cpp_lib_transparent_operators had too low a value"
#endif
int main( int, char *[] )
{
   return 0;
}
