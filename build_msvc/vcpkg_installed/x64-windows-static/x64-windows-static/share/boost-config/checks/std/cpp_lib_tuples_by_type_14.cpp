#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif
#include <utility>
#ifndef __cpp_lib_tuples_by_type
#error "Macro << __cpp_lib_tuples_by_type is not set"
#endif
#if __cpp_lib_tuples_by_type < 201304
#error "Macro __cpp_lib_tuples_by_type had too low a value"
#endif
int main( int, char *[] )
{
   return 0;
}