#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif
#include <forward_list>
#ifndef __cpp_lib_list_remove_return_type
#error "Macro << __cpp_lib_list_remove_return_type is not set"
#endif
#if __cpp_lib_list_remove_return_type < 201806
#error "Macro __cpp_lib_list_remove_return_type had too low a value"
#endif
int main( int, char *[] )
{
   return 0;
}
