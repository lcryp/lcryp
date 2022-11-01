#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif
#ifndef __cpp_constexpr
#error "Macro << __cpp_constexpr is not set"
#endif
#if __cpp_constexpr < 201304
#error "Macro __cpp_constexpr had too low a value"
#endif
int main( int, char *[] )
{
   return 0;
}
