#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif
#include <new>
#ifndef __cpp_lib_launder
#error "Macro << __cpp_lib_launder is not set"
#endif
#if __cpp_lib_launder < 201606
#error "Macro __cpp_lib_launder had too low a value"
#endif
int main( int, char *[] )
{
   return 0;
}
