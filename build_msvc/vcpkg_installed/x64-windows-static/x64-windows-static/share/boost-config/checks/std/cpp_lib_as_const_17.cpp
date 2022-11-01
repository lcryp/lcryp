#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif
#include <utility>
#ifndef __cpp_lib_as_const
#error "Macro << __cpp_lib_as_const is not set"
#endif
#if __cpp_lib_as_const < 201510
#error "Macro __cpp_lib_as_const had too low a value"
#endif
int main( int, char *[] )
{
   return 0;
}
