#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif
#include <type_traits>
#ifndef __cpp_lib_is_aggregate
#error "Macro << __cpp_lib_is_aggregate is not set"
#endif
#if __cpp_lib_is_aggregate < 201703
#error "Macro __cpp_lib_is_aggregate had too low a value"
#endif
int main( int, char *[] )
{
   return 0;
}
