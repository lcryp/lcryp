#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif
#ifndef __cpp_raw_strings
#error "Macro << __cpp_raw_strings is not set"
#endif
#if __cpp_raw_strings < 200710
#error "Macro __cpp_raw_strings had too low a value"
#endif
int main( int, char *[] )
{
   return 0;
}
