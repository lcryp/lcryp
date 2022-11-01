#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif
#ifndef __cpp_inheriting_constructors
#error "Macro << __cpp_inheriting_constructors is not set"
#endif
#if __cpp_inheriting_constructors < 201511
#error "Macro __cpp_inheriting_constructors had too low a value"
#endif
int main( int, char *[] )
{
   return 0;
}
