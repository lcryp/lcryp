#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif
#include <algorithm>
#ifndef __cpp_lib_robust_nonmodifying_seq_ops
#error "Macro << __cpp_lib_robust_nonmodifying_seq_ops is not set"
#endif
#if __cpp_lib_robust_nonmodifying_seq_ops < 201304
#error "Macro __cpp_lib_robust_nonmodifying_seq_ops had too low a value"
#endif
int main( int, char *[] )
{
   return 0;
}
