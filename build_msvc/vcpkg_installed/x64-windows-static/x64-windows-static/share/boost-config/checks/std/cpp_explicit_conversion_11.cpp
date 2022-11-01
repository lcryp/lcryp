#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif
#ifndef __cpp_explicit_conversion
#error "Macro << __cpp_explicit_conversion is not set"
#endif
#if __cpp_explicit_conversion < 200710
#error "Macro __cpp_explicit_conversion had too low a value"
#endif
int main( int, char *[] )
{
   return 0;
}
