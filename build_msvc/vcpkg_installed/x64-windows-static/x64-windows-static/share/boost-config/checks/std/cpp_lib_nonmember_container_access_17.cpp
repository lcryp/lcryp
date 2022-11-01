#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif
#include <iterator>
#ifndef __cpp_lib_nonmember_container_access
#error "Macro << __cpp_lib_nonmember_container_access is not set"
#endif
#if __cpp_lib_nonmember_container_access < 201411
#error "Macro __cpp_lib_nonmember_container_access had too low a value"
#endif
int main( int, char *[] )
{
   return 0;
}
