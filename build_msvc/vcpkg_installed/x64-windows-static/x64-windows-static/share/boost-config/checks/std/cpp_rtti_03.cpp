#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif
#ifndef __cpp_rtti
#error "Macro << __cpp_rtti is not set"
#endif
#if __cpp_rtti < 199711
#error "Macro __cpp_rtti had too low a value"
#endif
int main( int, char *[] )
{
   return 0;
}
