#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif
#ifndef __cpp_noexcept_function_type
#error "Macro << __cpp_noexcept_function_type is not set"
#endif
#if __cpp_noexcept_function_type < 201510
#error "Macro __cpp_noexcept_function_type had too low a value"
#endif
int main( int, char *[] )
{
   return 0;
}
