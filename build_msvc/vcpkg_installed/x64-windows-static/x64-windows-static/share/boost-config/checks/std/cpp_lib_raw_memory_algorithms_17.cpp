#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif
#include <memory>
#ifndef __cpp_lib_raw_memory_algorithms
#error "Macro << __cpp_lib_raw_memory_algorithms is not set"
#endif
#if __cpp_lib_raw_memory_algorithms < 201606
#error "Macro __cpp_lib_raw_memory_algorithms had too low a value"
#endif
int main( int, char *[] )
{
   return 0;
}
