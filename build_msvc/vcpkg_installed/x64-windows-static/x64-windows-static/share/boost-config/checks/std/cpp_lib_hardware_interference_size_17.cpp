#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif
#include <new>
#ifndef __cpp_lib_hardware_interference_size
#error "Macro << __cpp_lib_hardware_interference_size is not set"
#endif
#if __cpp_lib_hardware_interference_size < 201703
#error "Macro __cpp_lib_hardware_interference_size had too low a value"
#endif
int main( int, char *[] )
{
   return 0;
}
