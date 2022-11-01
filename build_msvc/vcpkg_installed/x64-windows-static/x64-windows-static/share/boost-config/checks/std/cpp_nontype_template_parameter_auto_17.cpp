#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif
#ifndef __cpp_nontype_template_parameter_auto
#error "Macro << __cpp_nontype_template_parameter_auto is not set"
#endif
#if __cpp_nontype_template_parameter_auto < 201606
#error "Macro __cpp_nontype_template_parameter_auto had too low a value"
#endif
int main( int, char *[] )
{
   return 0;
}
