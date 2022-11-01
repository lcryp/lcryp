#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif
#include <numeric>
#ifndef __cpp_lib_gcd_lcm
#error "Macro << __cpp_lib_gcd_lcm is not set"
#endif
#if __cpp_lib_gcd_lcm < 201606
#error "Macro __cpp_lib_gcd_lcm had too low a value"
#endif
int main( int, char *[] )
{
   return 0;
}
