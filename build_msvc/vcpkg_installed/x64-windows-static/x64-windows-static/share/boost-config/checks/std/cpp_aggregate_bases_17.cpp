#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif
#ifndef __cpp_aggregate_bases
#error "Macro << __cpp_aggregate_bases is not set"
#endif
#if __cpp_aggregate_bases < 201603
#error "Macro __cpp_aggregate_bases had too low a value"
#endif
int main( int, char *[] )
{
   return 0;
}
