#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif
#include <algorithm>
#ifndef __cpp_lib_parallel_algorithm
#error "Macro << __cpp_lib_parallel_algorithm is not set"
#endif
#if __cpp_lib_parallel_algorithm < 201603
#error "Macro __cpp_lib_parallel_algorithm had too low a value"
#endif
int main( int, char *[] )
{
   return 0;
}