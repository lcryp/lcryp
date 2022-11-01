#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif
#ifndef __cpp_user_defined_literals
#error "Macro << __cpp_user_defined_literals is not set"
#endif
#if __cpp_user_defined_literals < 200809
#error "Macro __cpp_user_defined_literals had too low a value"
#endif
int main( int, char *[] )
{
   return 0;
}
