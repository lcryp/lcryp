#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif
#include <memory>
#ifndef __cpp_lib_addressof_constexpr
#error "Macro << __cpp_lib_addressof_constexpr is not set"
#endif
#if __cpp_lib_addressof_constexpr < 201603
#error "Macro __cpp_lib_addressof_constexpr had too low a value"
#endif
int main( int, char *[] )
{
   return 0;
}
