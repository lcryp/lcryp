#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif
#include <mutex>
#ifndef __cpp_lib_scoped_lock
#error "Macro << __cpp_lib_scoped_lock is not set"
#endif
#if __cpp_lib_scoped_lock < 201703
#error "Macro __cpp_lib_scoped_lock had too low a value"
#endif
int main( int, char *[] )
{
   return 0;
}
