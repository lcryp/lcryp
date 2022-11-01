#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif
#include <memory>
#ifndef __cpp_lib_make_unique
#error "Macro << __cpp_lib_make_unique is not set"
#endif
#if __cpp_lib_make_unique < 201304
#error "Macro __cpp_lib_make_unique had too low a value"
#endif
int main( int, char *[] )
{
   return 0;
}
