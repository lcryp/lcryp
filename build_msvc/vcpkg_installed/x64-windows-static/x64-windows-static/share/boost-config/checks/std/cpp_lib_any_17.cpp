#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif
#include <any>
#ifndef __cpp_lib_any
#error "Macro << __cpp_lib_any is not set"
#endif
#if __cpp_lib_any < 201606
#error "Macro __cpp_lib_any had too low a value"
#endif
int main( int, char *[] )
{
   return 0;
}
