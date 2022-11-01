#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif
#include <execution>
#ifndef __cpp_lib_execution
#error "Macro << __cpp_lib_execution is not set"
#endif
#if __cpp_lib_execution < 201603
#error "Macro __cpp_lib_execution had too low a value"
#endif
int main( int, char *[] )
{
   return 0;
}
