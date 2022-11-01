#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif
#include <tuple>
#ifndef __cpp_lib_make_from_tuple
#error "Macro << __cpp_lib_make_from_tuple is not set"
#endif
#if __cpp_lib_make_from_tuple < 201606
#error "Macro __cpp_lib_make_from_tuple had too low a value"
#endif
int main( int, char *[] )
{
   return 0;
}
