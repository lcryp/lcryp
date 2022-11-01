#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif
#include <map>
#ifndef __cpp_lib_generic_associative_lookup
#error "Macro << __cpp_lib_generic_associative_lookup is not set"
#endif
#if __cpp_lib_generic_associative_lookup < 201304
#error "Macro __cpp_lib_generic_associative_lookup had too low a value"
#endif
int main( int, char *[] )
{
   return 0;
}
