#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif
#ifndef __cpp_sized_deallocation
#error "Macro << __cpp_sized_deallocation is not set"
#endif
#if __cpp_sized_deallocation < 201309
#error "Macro __cpp_sized_deallocation had too low a value"
#endif
int main( int, char *[] )
{
   return 0;
}
