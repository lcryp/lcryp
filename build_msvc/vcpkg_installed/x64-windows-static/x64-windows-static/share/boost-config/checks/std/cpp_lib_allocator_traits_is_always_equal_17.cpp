#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif
#include <memory>
#ifndef __cpp_lib_allocator_traits_is_always_equal
#error "Macro << __cpp_lib_allocator_traits_is_always_equal is not set"
#endif
#if __cpp_lib_allocator_traits_is_always_equal < 201411
#error "Macro __cpp_lib_allocator_traits_is_always_equal had too low a value"
#endif
int main( int, char *[] )
{
   return 0;
}
