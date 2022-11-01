#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif
#include <type_traits>
#ifndef __cpp_lib_is_swappable
#error "Macro << __cpp_lib_is_swappable is not set"
#endif
#if __cpp_lib_is_swappable < 201603
#error "Macro __cpp_lib_is_swappable had too low a value"
#endif
int main( int, char *[] )
{
   return 0;
}
