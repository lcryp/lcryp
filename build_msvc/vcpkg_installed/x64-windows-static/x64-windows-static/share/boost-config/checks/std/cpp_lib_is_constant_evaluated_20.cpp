#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif
#include <type_traits>
#ifndef __cpp_lib_is_constant_evaluated
#error "Macro << __cpp_lib_is_constant_evaluated is not set"
#endif
#if __cpp_lib_is_constant_evaluated < 201811
#error "Macro __cpp_lib_is_constant_evaluated had too low a value"
#endif
int main( int, char *[] )
{
   return 0;
}
