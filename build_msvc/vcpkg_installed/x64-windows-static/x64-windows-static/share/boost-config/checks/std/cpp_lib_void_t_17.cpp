#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif
#include <type_traits>
#ifndef __cpp_lib_void_t
#error "Macro << __cpp_lib_void_t is not set"
#endif
#if __cpp_lib_void_t < 201411
#error "Macro __cpp_lib_void_t had too low a value"
#endif
int main( int, char *[] )
{
   return 0;
}
