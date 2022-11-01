#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif
#ifndef __cpp_threadsafe_static_init
#error "Macro << __cpp_threadsafe_static_init is not set"
#endif
#if __cpp_threadsafe_static_init < 200806
#error "Macro __cpp_threadsafe_static_init had too low a value"
#endif
int main( int, char *[] )
{
   return 0;
}
