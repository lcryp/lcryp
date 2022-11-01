#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif
#ifndef __cpp_generic_lambdas
#error "Macro << __cpp_generic_lambdas is not set"
#endif
#if __cpp_generic_lambdas < 201304
#error "Macro __cpp_generic_lambdas had too low a value"
#endif
int main( int, char *[] )
{
   return 0;
}
