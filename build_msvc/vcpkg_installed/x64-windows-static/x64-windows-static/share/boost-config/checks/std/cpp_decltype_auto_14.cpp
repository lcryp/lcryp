#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif
#ifndef __cpp_decltype_auto
#error "Macro << __cpp_decltype_auto is not set"
#endif
#if __cpp_decltype_auto < 201304
#error "Macro __cpp_decltype_auto had too low a value"
#endif
int main( int, char *[] )
{
   return 0;
}
