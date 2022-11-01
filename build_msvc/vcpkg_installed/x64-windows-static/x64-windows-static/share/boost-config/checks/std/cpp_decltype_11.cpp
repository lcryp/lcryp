#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif
#ifndef __cpp_decltype
#error "Macro << __cpp_decltype is not set"
#endif
#if __cpp_decltype < 200707
#error "Macro __cpp_decltype had too low a value"
#endif
int main( int, char *[] )
{
   return 0;
}
