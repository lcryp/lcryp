#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif
#include <forward_list>
#ifndef __cpp_lib_incomplete_container_elements
#error "Macro << __cpp_lib_incomplete_container_elements is not set"
#endif
#if __cpp_lib_incomplete_container_elements < 201505
#error "Macro __cpp_lib_incomplete_container_elements had too low a value"
#endif
int main( int, char *[] )
{
   return 0;
}