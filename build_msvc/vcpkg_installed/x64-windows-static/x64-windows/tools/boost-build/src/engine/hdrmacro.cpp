#include "jam.h"
#include "hdrmacro.h"
#include "compile.h"
#include "hash.h"
#include "lists.h"
#include "object.h"
#include "parse.h"
#include "rules.h"
#include "jam_strings.h"
#include "subst.h"
#include "variable.h"
#include "output.h"
#include <errno.h>
#include <string.h>
typedef struct header_macro
{
  OBJECT * symbol;
  OBJECT * filename;
} HEADER_MACRO;
static struct hash * header_macros_hash = 0;
#define MAXINC 10
void macro_headers( TARGET * t )
{
    static regexp * re = 0;
    FILE * f;
    char buf[ 1024 ];
    if ( DEBUG_HEADER )
        out_printf( "macro header scan for %s\n", object_str( t->name ) );
    if ( !re )
    {
        OBJECT * re_str = object_new(
            "^[     ]*#[    ]*define[   ]*([A-Za-z][A-Za-z0-9_]*)[  ]*"
            "[<\"]([^\">]*)[\">].*$" );
        re = regex_compile( re_str );
        object_free( re_str );
    }
    if ( !( f = fopen( object_str( t->boundname ), "r" ) ) )
    {
        err_printf( "[errno %d] failed to scan include file '%s': %s",
            errno, object_str( t->boundname ), strerror(errno) );
        return;
    }
    while ( fgets( buf, sizeof( buf ), f ) )
    {
        HEADER_MACRO var;
        HEADER_MACRO * v = &var;
        if ( regexec( re, buf ) && re->startp[ 1 ] )
        {
            OBJECT * symbol;
            int found;
            ( (char *)re->endp[ 1 ] )[ 0 ] = '\0';
            ( (char *)re->endp[ 2 ] )[ 0 ] = '\0';
            if ( DEBUG_HEADER )
                out_printf( "macro '%s' used to define filename '%s' in '%s'\n",
                    re->startp[ 1 ], re->startp[ 2 ], object_str( t->boundname )
                    );
            if ( !header_macros_hash )
                header_macros_hash = hashinit( sizeof( HEADER_MACRO ),
                    "hdrmacros" );
            symbol = object_new( re->startp[ 1 ] );
            v = (HEADER_MACRO *)hash_insert( header_macros_hash, symbol, &found
                );
            if ( !found )
            {
                v->symbol = symbol;
                v->filename = object_new( re->startp[ 2 ] );
            }
            else
                object_free( symbol );
        }
    }
    fclose( f );
}
OBJECT * macro_header_get( OBJECT * macro_name )
{
    HEADER_MACRO * v;
    if ( header_macros_hash && ( v = (HEADER_MACRO *)hash_find(
        header_macros_hash, macro_name ) ) )
    {
        if ( DEBUG_HEADER )
            out_printf( "### macro '%s' evaluated to '%s'\n", object_str( macro_name
                ), object_str( v->filename ) );
        return v->filename;
    }
    return 0;
}
