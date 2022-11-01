#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif
#include <memory>
#ifndef __cpp_lib_shared_ptr_arrays
#error "Macro << __cpp_lib_shared_ptr_arrays is not set"
#endif
#if __cpp_lib_shared_ptr_arrays < 201611
#error "Macro __cpp_lib_shared_ptr_arrays had too low a value"
#endif
int main( int, char *[] )
{
   return 0;
}
