#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif
#include <array>
#ifndef __cpp_lib_constexpr_misc
#error "Macro << __cpp_lib_constexpr_misc is not set"
#endif
#if __cpp_lib_constexpr_misc < 201811
#error "Macro __cpp_lib_constexpr_misc had too low a value"
#endif
int main( int, char *[] )
{
   return 0;
}
