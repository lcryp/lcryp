#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif
#ifndef __cpp_attributes
#error "Macro << __cpp_attributes is not set"
#endif
#if __cpp_attributes < 200809
#error "Macro __cpp_attributes had too low a value"
#endif
int main( int, char *[] )
{
   return 0;
}
