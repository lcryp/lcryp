#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif
#include <type_traits>
#ifndef __cpp_lib_logical_traits
#error "Macro << __cpp_lib_logical_traits is not set"
#endif
#if __cpp_lib_logical_traits < 201510
#error "Macro __cpp_lib_logical_traits had too low a value"
#endif
int main( int, char *[] )
{
   return 0;
}
