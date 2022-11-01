#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif
#include <type_traits>
#ifndef __cpp_lib_is_null_pointer
#error "Macro << __cpp_lib_is_null_pointer is not set"
#endif
#if __cpp_lib_is_null_pointer < 201309
#error "Macro __cpp_lib_is_null_pointer had too low a value"
#endif
int main( int, char *[] )
{
   return 0;
}
