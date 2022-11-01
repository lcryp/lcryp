#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif
#include <memory_resource>
#ifndef __cpp_lib_memory_resource
#error "Macro << __cpp_lib_memory_resource is not set"
#endif
#if __cpp_lib_memory_resource < 201603
#error "Macro __cpp_lib_memory_resource had too low a value"
#endif
int main( int, char *[] )
{
   return 0;
}
