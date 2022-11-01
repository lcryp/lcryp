#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif
#include <functional>
#ifndef __cpp_lib_invoke
#error "Macro << __cpp_lib_invoke is not set"
#endif
#if __cpp_lib_invoke < 201411
#error "Macro __cpp_lib_invoke had too low a value"
#endif
int main( int, char *[] )
{
   return 0;
}
