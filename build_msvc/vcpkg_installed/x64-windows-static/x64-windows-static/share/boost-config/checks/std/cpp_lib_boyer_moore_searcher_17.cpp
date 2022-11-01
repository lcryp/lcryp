#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif
#include <functional>
#ifndef __cpp_lib_boyer_moore_searcher
#error "Macro << __cpp_lib_boyer_moore_searcher is not set"
#endif
#if __cpp_lib_boyer_moore_searcher < 201603
#error "Macro __cpp_lib_boyer_moore_searcher had too low a value"
#endif
int main( int, char *[] )
{
   return 0;
}
