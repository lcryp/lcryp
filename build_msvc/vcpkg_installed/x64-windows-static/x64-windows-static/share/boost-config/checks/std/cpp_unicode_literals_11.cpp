#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif
#ifndef __cpp_unicode_literals
#error "Macro << __cpp_unicode_literals is not set"
#endif
#if __cpp_unicode_literals < 200710
#error "Macro __cpp_unicode_literals had too low a value"
#endif
int main( int, char *[] )
{
   return 0;
}
