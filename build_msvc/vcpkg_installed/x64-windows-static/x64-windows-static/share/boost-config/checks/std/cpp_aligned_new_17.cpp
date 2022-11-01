#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif
#ifndef __cpp_aligned_new
#error "Macro << __cpp_aligned_new is not set"
#endif
#if __cpp_aligned_new < 201606
#error "Macro __cpp_aligned_new had too low a value"
#endif
int main( int, char *[] )
{
   return 0;
}
