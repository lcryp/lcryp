#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif
#ifndef __cpp_binary_literals
#error "Macro << __cpp_binary_literals is not set"
#endif
#if __cpp_binary_literals < 201304
#error "Macro __cpp_binary_literals had too low a value"
#endif
int main( int, char *[] )
{
   return 0;
}
