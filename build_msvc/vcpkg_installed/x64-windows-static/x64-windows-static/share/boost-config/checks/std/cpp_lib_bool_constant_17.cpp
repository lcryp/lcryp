#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif
#include <type_traits>
#ifndef __cpp_lib_bool_constant
#error "Macro << __cpp_lib_bool_constant is not set"
#endif
#if __cpp_lib_bool_constant < 201505
#error "Macro __cpp_lib_bool_constant had too low a value"
#endif
int main( int, char *[] )
{
   return 0;
}
