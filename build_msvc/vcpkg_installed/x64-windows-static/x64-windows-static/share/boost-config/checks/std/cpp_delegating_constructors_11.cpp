#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif
#ifndef __cpp_delegating_constructors
#error "Macro << __cpp_delegating_constructors is not set"
#endif
#if __cpp_delegating_constructors < 200604
#error "Macro __cpp_delegating_constructors had too low a value"
#endif
int main( int, char *[] )
{
   return 0;
}
