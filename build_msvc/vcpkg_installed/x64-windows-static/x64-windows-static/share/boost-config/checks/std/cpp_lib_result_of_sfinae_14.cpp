#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif
#include <functional>
#ifndef __cpp_lib_result_of_sfinae
#error "Macro << __cpp_lib_result_of_sfinae is not set"
#endif
#if __cpp_lib_result_of_sfinae < 201210
#error "Macro __cpp_lib_result_of_sfinae had too low a value"
#endif
int main( int, char *[] )
{
   return 0;
}
