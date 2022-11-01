#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif
#ifndef __cpp_deduction_guides
#error "Macro << __cpp_deduction_guides is not set"
#endif
#if __cpp_deduction_guides < 201611
#error "Macro __cpp_deduction_guides had too low a value"
#endif
int main( int, char *[] )
{
   return 0;
}
