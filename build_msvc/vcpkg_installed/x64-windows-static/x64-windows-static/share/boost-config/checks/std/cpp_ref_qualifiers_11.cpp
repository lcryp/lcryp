#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif
#ifndef __cpp_ref_qualifiers
#error "Macro << __cpp_ref_qualifiers is not set"
#endif
#if __cpp_ref_qualifiers < 200710
#error "Macro __cpp_ref_qualifiers had too low a value"
#endif
int main( int, char *[] )
{
   return 0;
}