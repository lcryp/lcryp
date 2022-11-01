#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif
#ifndef __cpp_rvalue_references
#error "Macro << __cpp_rvalue_references is not set"
#endif
#if __cpp_rvalue_references < 200610
#error "Macro __cpp_rvalue_references had too low a value"
#endif
int main( int, char *[] )
{
   return 0;
}
