#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif
#ifndef __cpp_fold_expressions
#error "Macro << __cpp_fold_expressions is not set"
#endif
#if __cpp_fold_expressions < 201603
#error "Macro __cpp_fold_expressions had too low a value"
#endif
int main( int, char *[] )
{
   return 0;
}
