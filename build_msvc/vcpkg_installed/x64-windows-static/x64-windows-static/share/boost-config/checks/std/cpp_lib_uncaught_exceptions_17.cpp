#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif
#include <exception>
#ifndef __cpp_lib_uncaught_exceptions
#error "Macro << __cpp_lib_uncaught_exceptions is not set"
#endif
#if __cpp_lib_uncaught_exceptions < 201411
#error "Macro __cpp_lib_uncaught_exceptions had too low a value"
#endif
int main( int, char *[] )
{
   return 0;
}