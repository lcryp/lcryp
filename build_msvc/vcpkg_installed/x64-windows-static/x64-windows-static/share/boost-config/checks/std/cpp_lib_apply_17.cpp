#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif
#include <tuple>
#ifndef __cpp_lib_apply
#error "Macro << __cpp_lib_apply is not set"
#endif
#if __cpp_lib_apply < 201603
#error "Macro __cpp_lib_apply had too low a value"
#endif
int main( int, char *[] )
{
   return 0;
}