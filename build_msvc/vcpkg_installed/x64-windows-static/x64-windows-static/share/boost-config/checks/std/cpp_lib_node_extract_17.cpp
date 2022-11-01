#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif
#include <map>
#ifndef __cpp_lib_node_extract
#error "Macro << __cpp_lib_node_extract is not set"
#endif
#if __cpp_lib_node_extract < 201606
#error "Macro __cpp_lib_node_extract had too low a value"
#endif
int main( int, char *[] )
{
   return 0;
}
