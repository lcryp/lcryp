#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif
#ifndef __cpp_range_based_for
#error "Macro << __cpp_range_based_for is not set"
#endif
#if __cpp_range_based_for < 201603
#error "Macro __cpp_range_based_for had too low a value"
#endif
int main( int, char *[] )
{
   return 0;
}
