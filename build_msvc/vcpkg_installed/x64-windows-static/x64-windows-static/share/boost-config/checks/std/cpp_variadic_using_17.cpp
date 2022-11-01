#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif
#ifndef __cpp_variadic_using
#error "Macro << __cpp_variadic_using is not set"
#endif
#if __cpp_variadic_using < 201611
#error "Macro __cpp_variadic_using had too low a value"
#endif
int main( int, char *[] )
{
   return 0;
}
