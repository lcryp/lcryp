#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif
#ifndef __cpp_conditional_explicit
#error "Macro << __cpp_conditional_explicit is not set"
#endif
#if __cpp_conditional_explicit < 201806
#error "Macro __cpp_conditional_explicit had too low a value"
#endif
int main( int, char *[] )
{
   return 0;
}
