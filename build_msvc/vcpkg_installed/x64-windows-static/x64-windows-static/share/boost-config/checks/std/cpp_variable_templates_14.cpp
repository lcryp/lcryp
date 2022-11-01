#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif
#ifndef __cpp_variable_templates
#error "Macro << __cpp_variable_templates is not set"
#endif
#if __cpp_variable_templates < 201304
#error "Macro __cpp_variable_templates had too low a value"
#endif
int main( int, char *[] )
{
   return 0;
}
