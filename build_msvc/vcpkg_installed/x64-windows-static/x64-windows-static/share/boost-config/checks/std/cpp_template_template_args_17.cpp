#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif
#ifndef __cpp_template_template_args
#error "Macro << __cpp_template_template_args is not set"
#endif
#if __cpp_template_template_args < 201611
#error "Macro __cpp_template_template_args had too low a value"
#endif
int main( int, char *[] )
{
   return 0;
}
