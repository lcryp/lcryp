#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif
#include <iterator>
#ifndef __cpp_lib_null_iterators
#error "Macro << __cpp_lib_null_iterators is not set"
#endif
#if __cpp_lib_null_iterators < 201304
#error "Macro __cpp_lib_null_iterators had too low a value"
#endif
int main( int, char *[] )
{
   return 0;
}
