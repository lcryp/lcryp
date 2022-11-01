#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif
#ifndef __cpp_initializer_lists
#error "Macro << __cpp_initializer_lists is not set"
#endif
#if __cpp_initializer_lists < 200806
#error "Macro __cpp_initializer_lists had too low a value"
#endif
int main( int, char *[] )
{
   return 0;
}
