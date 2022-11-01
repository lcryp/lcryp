# include "jam.h"
# define CHECK_BIT( tab, bit ) ( tab[ (bit)/8 ] & (1<<( (bit)%8 )) )
# define BITLISTSIZE 16
static void globchars( const char * s, const char * e, char * b );
int glob( const char * c, const char * s )
{
    char   bitlist[ BITLISTSIZE ];
    const char * here;
    for ( ; ; )
    switch ( *c++ )
    {
    case '\0':
        return *s ? -1 : 0;
    case '?':
        if ( !*s++ )
            return 1;
        break;
    case '[':
        here = c;
        do if ( !*c++ ) return 1;
        while ( ( here == c ) || ( *c != ']' ) );
        ++c;
        globchars( here, c, bitlist );
        if ( !CHECK_BIT( bitlist, *(const unsigned char *)s ) )
            return 1;
        ++s;
        break;
    case '*':
        here = s;
        while ( *s )
            ++s;
        while ( s != here )
        {
            int r;
            r = *c ? glob( c, s ) : *s ? -1 : 0;
            if ( !r )
                return 0;
            if ( r < 0 )
                return 1;
            --s;
        }
        break;
    case '\\':
        if ( !*c || ( *s++ != *c++ ) )
            return 1;
        break;
    default:
        if ( *s++ != c[ -1 ] )
            return 1;
        break;
    }
}
static void globchars( const char * s,  const char * e, char * b )
{
    int neg = 0;
    memset( b, '\0', BITLISTSIZE  );
    if ( *s == '^' )
    {
        ++neg;
        ++s;
    }
    while ( s < e )
    {
        int c;
        if ( ( s + 2 < e ) && ( s[1] == '-' ) )
        {
            for ( c = s[0]; c <= s[2]; ++c )
                b[ c/8 ] |= ( 1 << ( c % 8 ) );
            s += 3;
        }
        else
        {
            c = *s++;
            b[ c/8 ] |= ( 1 << ( c % 8 ) );
        }
    }
    if ( neg )
    {
        int i;
        for ( i = 0; i < BITLISTSIZE; ++i )
            b[ i ] ^= 0377;
    }
    b[0] &= 0376;
}
