#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif
#ifndef __cpp_init_captures
#error "Macro << __cpp_init_captures is not set"
#endif
#if __cpp_init_captures < 201304
#error "Macro __cpp_init_captures had too low a value"
#endif
int main( int, char *[] )
{
   return 0;
}
