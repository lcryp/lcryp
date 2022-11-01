#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif
#include <concepts>
#ifndef __cpp_lib_concepts
#error "Macro << __cpp_lib_concepts is not set"
#endif
#if __cpp_lib_concepts < 201806
#error "Macro __cpp_lib_concepts had too low a value"
#endif
int main( int, char *[] )
{
   return 0;
}
