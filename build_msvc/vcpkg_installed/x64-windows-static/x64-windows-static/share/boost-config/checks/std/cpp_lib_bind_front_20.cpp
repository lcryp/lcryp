#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif
#include <functional>
#ifndef __cpp_lib_bind_front
#error "Macro << __cpp_lib_bind_front is not set"
#endif
#if __cpp_lib_bind_front < 201811
#error "Macro __cpp_lib_bind_front had too low a value"
#endif
int main( int, char *[] )
{
   return 0;
}
