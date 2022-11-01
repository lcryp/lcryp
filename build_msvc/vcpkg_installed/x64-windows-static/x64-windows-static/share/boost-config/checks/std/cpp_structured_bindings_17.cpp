#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif
#ifndef __cpp_structured_bindings
#error "Macro << __cpp_structured_bindings is not set"
#endif
#if __cpp_structured_bindings < 201606
#error "Macro __cpp_structured_bindings had too low a value"
#endif
int main( int, char *[] )
{
   return 0;
}
