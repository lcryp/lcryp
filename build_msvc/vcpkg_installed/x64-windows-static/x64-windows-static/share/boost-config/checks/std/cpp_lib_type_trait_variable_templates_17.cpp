#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif
#include <type_traits>
#ifndef __cpp_lib_type_trait_variable_templates
#error "Macro << __cpp_lib_type_trait_variable_templates is not set"
#endif
#if __cpp_lib_type_trait_variable_templates < 201510
#error "Macro __cpp_lib_type_trait_variable_templates had too low a value"
#endif
int main( int, char *[] )
{
   return 0;
}