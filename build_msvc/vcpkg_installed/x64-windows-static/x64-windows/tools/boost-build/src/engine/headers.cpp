#include "jam.h"
#include "headers.h"
#include "compile.h"
#include "hdrmacro.h"
#include "lists.h"
#include "modules.h"
#include "object.h"
#include "parse.h"
#include "rules.h"
#include "subst.h"
#include "variable.h"
#include "output.h"
#ifdef OPT_HEADER_CACHE_EXT
# include "hcache.h"
#endif
#include <errno.h>
#include <string.h>
#ifndef OPT_HEADER_CACHE_EXT
static LIST * headers1( LIST *, OBJECT * file, int rec, regexp * re[] );
#endif
#define MAXINC 10
void headers( TARGET * t )
{
    LIST   * hdrscan;
    LIST   * hdrrule;
    #ifndef OPT_HEADER_CACHE_EXT
    LIST   * headlist = L0;
    #endif
    regexp * re[ MAXINC ];
    int rec = 0;
    LISTITER iter;
    LISTITER end;
    hdrscan = var_get( root_module(), constant_HDRSCAN );
    if ( list_empty( hdrscan ) )
        return;
    hdrrule = var_get( root_module(), constant_HDRRULE );
    if ( list_empty( hdrrule ) )
        return;
    if ( DEBUG_HEADER )
        out_printf( "header scan %s\n", object_str( t->name ) );
    iter = list_begin( hdrscan );
    end = list_end( hdrscan );
    for ( ; ( rec < MAXINC ) && iter != end; iter = list_next( iter ) )
    {
        re[ rec++ ] = regex_compile( list_item( iter ) );
    }
    {
        FRAME frame[ 1 ];
        frame_init( frame );
        lol_add( frame->args, list_new( object_copy( t->name ) ) );
#ifdef OPT_HEADER_CACHE_EXT
        lol_add( frame->args, hcache( t, rec, re, hdrscan ) );
#else
        lol_add( frame->args, headers1( headlist, t->boundname, rec, re ) );
#endif
        if ( lol_get( frame->args, 1 ) )
        {
            OBJECT * rulename = list_front( hdrrule );
            lol_add( frame->args, list_new( object_copy( t->boundname ) ) );
            list_free( evaluate_rule( bindrule( rulename, frame->module ), rulename, frame ) );
        }
        frame_free( frame );
    }
}
#ifndef OPT_HEADER_CACHE_EXT
static
#endif
LIST * headers1( LIST * l, OBJECT * file, int rec, regexp * re[] )
{
    FILE * f;
    char buf[ 1024 ];
    int i;
    static regexp * re_macros = 0;
#ifdef OPT_IMPROVED_PATIENCE_EXT
    static int count = 0;
    ++count;
    if ( ( ( count == 100 ) || !( count % 1000 ) ) && DEBUG_MAKE )
    {
        out_printf( "...patience...\n" );
        out_flush();
    }
#endif
    if ( re_macros == 0 )
    {
        OBJECT * re_str = object_new(
            "#[ \t]*include[ \t]*([A-Za-z][A-Za-z0-9_]*).*$" );
        re_macros = regex_compile( re_str );
        object_free( re_str );
    }
    if ( !( f = fopen( object_str( file ), "r" ) ) )
    {
        if ( !globs.noexec || errno != ENOENT )
            err_printf( "[errno %d] failed to scan file '%s': %s",
                errno, object_str( file ), strerror(errno) );
        return l;
    }
    while ( fgets( buf, sizeof( buf ), f ) )
    {
        for ( i = 0; i < rec; ++i )
            if ( regexec( re[ i ], buf ) && re[ i ]->startp[ 1 ] )
            {
                ( (char *)re[ i ]->endp[ 1 ] )[ 0 ] = '\0';
                if ( DEBUG_HEADER )
                    out_printf( "header found: %s\n", re[ i ]->startp[ 1 ] );
                l = list_push_back( l, object_new( re[ i ]->startp[ 1 ] ) );
            }
        if ( regexec( re_macros, buf ) && re_macros->startp[ 1 ] )
        {
            OBJECT * header_filename;
            OBJECT * macro_name;
            ( (char *)re_macros->endp[ 1 ] )[ 0 ] = '\0';
            if ( DEBUG_HEADER )
                out_printf( "macro header found: %s", re_macros->startp[ 1 ] );
            macro_name = object_new( re_macros->startp[ 1 ] );
            header_filename = macro_header_get( macro_name );
            object_free( macro_name );
            if ( header_filename )
            {
                if ( DEBUG_HEADER )
                    out_printf( " resolved to '%s'\n", object_str( header_filename )
                        );
                l = list_push_back( l, object_copy( header_filename ) );
            }
            else
            {
                if ( DEBUG_HEADER )
                    out_printf( " ignored !!\n" );
            }
        }
    }
    fclose( f );
    return l;
}
void regerror( char const * s )
{
    out_printf( "re error %s\n", s );
}
