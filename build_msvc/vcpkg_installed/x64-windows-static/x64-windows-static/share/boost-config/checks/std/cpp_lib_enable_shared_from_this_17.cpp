#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif
#include <memory>
#ifndef __cpp_lib_enable_shared_from_this
#error "Macro << __cpp_lib_enable_shared_from_this is not set"
#endif
#if __cpp_lib_enable_shared_from_this < 201603
#error "Macro __cpp_lib_enable_shared_from_this had too low a value"
#endif
int main( int, char *[] )
{
   return 0;
}
