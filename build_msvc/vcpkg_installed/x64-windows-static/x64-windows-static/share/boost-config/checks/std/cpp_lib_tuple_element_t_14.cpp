#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif
#include <tuple>
#ifndef __cpp_lib_tuple_element_t
#error "Macro << __cpp_lib_tuple_element_t is not set"
#endif
#if __cpp_lib_tuple_element_t < 201402
#error "Macro __cpp_lib_tuple_element_t had too low a value"
#endif
int main( int, char *[] )
{
   return 0;
}
