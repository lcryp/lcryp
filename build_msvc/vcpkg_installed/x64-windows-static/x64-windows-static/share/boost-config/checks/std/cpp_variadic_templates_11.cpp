#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif
#ifndef __cpp_variadic_templates
#error "Macro << __cpp_variadic_templates is not set"
#endif
#if __cpp_variadic_templates < 200704
#error "Macro __cpp_variadic_templates had too low a value"
#endif
int main( int, char *[] )
{
   return 0;
}
