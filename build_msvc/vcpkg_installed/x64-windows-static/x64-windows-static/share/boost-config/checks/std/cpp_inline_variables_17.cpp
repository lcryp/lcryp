#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif
#ifndef __cpp_inline_variables
#error "Macro << __cpp_inline_variables is not set"
#endif
#if __cpp_inline_variables < 201606
#error "Macro __cpp_inline_variables had too low a value"
#endif
int main( int, char *[] )
{
   return 0;
}
