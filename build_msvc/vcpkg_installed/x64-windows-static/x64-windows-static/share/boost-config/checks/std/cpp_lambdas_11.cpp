#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif
#ifndef __cpp_lambdas
#error "Macro << __cpp_lambdas is not set"
#endif
#if __cpp_lambdas < 200907
#error "Macro __cpp_lambdas had too low a value"
#endif
int main( int, char *[] )
{
   return 0;
}
