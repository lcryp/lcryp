#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif
#ifndef __cpp_impl_three_way_comparison
#error "Macro << __cpp_impl_three_way_comparison is not set"
#endif
#if __cpp_impl_three_way_comparison < 201711
#error "Macro __cpp_impl_three_way_comparison had too low a value"
#endif
int main( int, char *[] )
{
   return 0;
}