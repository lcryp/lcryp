#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif
#include <unordered_map>
#ifndef __cpp_lib_generic_unordered_lookup
#error "Macro << __cpp_lib_generic_unordered_lookup is not set"
#endif
#if __cpp_lib_generic_unordered_lookup < 201811
#error "Macro __cpp_lib_generic_unordered_lookup had too low a value"
#endif
int main( int, char *[] )
{
   return 0;
}
