#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif
#ifndef __cpp_static_assert
#error "Macro << __cpp_static_assert is not set"
#endif
#if __cpp_static_assert < 201411
#error "Macro __cpp_static_assert had too low a value"
#endif
int main( int, char *[] )
{
   return 0;
}
