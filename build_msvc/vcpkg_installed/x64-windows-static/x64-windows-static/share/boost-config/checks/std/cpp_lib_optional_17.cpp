#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif
#include <optional>
#ifndef __cpp_lib_optional
#error "Macro << __cpp_lib_optional is not set"
#endif
#if __cpp_lib_optional < 201606
#error "Macro __cpp_lib_optional had too low a value"
#endif
int main( int, char *[] )
{
   return 0;
}
