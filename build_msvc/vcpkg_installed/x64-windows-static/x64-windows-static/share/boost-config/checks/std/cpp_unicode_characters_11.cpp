#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif
#ifndef __cpp_unicode_characters
#error "Macro << __cpp_unicode_characters is not set"
#endif
#if __cpp_unicode_characters < 200704
#error "Macro __cpp_unicode_characters had too low a value"
#endif
int main( int, char *[] )
{
   return 0;
}