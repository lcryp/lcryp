#include "jam.h"
#include "variable.h"
#include "filesys.h"
#include "hash.h"
#include "modules.h"
#include "parse.h"
#include "pathsys.h"
#include "jam_strings.h"
#include "output.h"
#include <stdio.h>
#include <stdlib.h>
typedef struct _variable VARIABLE ;
struct _variable
{
    OBJECT * symbol;
    LIST   * value;
};
static LIST * * var_enter( struct module_t *, OBJECT * symbol );
static void var_dump( OBJECT * symbol, LIST * value, const char * what );
void var_defines( struct module_t * module, const char * const * e, int preprocess )
{
    string buf[ 1 ];
    string_new( buf );
    for ( ; *e; ++e )
    {
        const char * val;
        if ( ( val = strchr( *e, '=' ) )
#if defined( OS_MAC )
            || ( val = *e + strlen( *e ) )
#endif
        )
        {
            LIST * l = L0;
            int32_t const len = int32_t(strlen( val + 1 ));
            int const quoted = ( val[ 1 ] == '"' ) && ( val[ len ] == '"' ) &&
                ( len > 1 );
            if ( quoted && preprocess )
            {
                string_append_range( buf, val + 2, val + len );
                l = list_push_back( l, object_new( buf->value ) );
                string_truncate( buf, 0 );
            }
            else
            {
                const char * p;
                const char * pp;
                char split =
#if defined( OPT_NO_EXTERNAL_VARIABLE_SPLIT )
                    '\0'
#elif defined( OS_MAC )
                    ','
#else
                    ' '
#endif
                    ;
                if ( val - 4 >= *e )
                {
                    if ( !strncmp( val - 4, "PATH", 4 ) ||
                        !strncmp( val - 4, "Path", 4 ) ||
                        !strncmp( val - 4, "path", 4 ) )
                        split = SPLITPATH;
                }
                for
                (
                    pp = val + 1;
                    preprocess && ( ( p = strchr( pp, split ) ) != 0 );
                    pp = p + 1
                )
                {
                    string_append_range( buf, pp, p );
                    l = list_push_back( l, object_new( buf->value ) );
                    string_truncate( buf, 0 );
                }
                l = list_push_back( l, object_new( pp ) );
            }
            string_append_range( buf, *e, val );
            {
                OBJECT * varname = object_new( buf->value );
                var_set( module, varname, l, VAR_SET );
                object_free( varname );
            }
            string_truncate( buf, 0 );
        }
    }
    string_free( buf );
}
static LIST * saved_var = L0;
LIST * var_get( struct module_t * module, OBJECT * symbol )
{
    LIST * result = L0;
#ifdef OPT_AT_FILES
    if ( object_equal( symbol, constant_TMPDIR ) )
    {
        list_free( saved_var );
        result = saved_var = list_new( object_new( path_tmpdir()->value ) );
    }
    else if ( object_equal( symbol, constant_TMPNAME ) )
    {
        list_free( saved_var );
        result = saved_var = list_new( path_tmpnam() );
    }
    else if ( object_equal( symbol, constant_TMPFILE ) )
    {
        list_free( saved_var );
        result = saved_var = list_new( path_tmpfile() );
    }
    else if ( object_equal( symbol, constant_STDOUT ) )
    {
        list_free( saved_var );
        result = saved_var = list_new( object_copy( constant_STDOUT ) );
    }
    else if ( object_equal( symbol, constant_STDERR ) )
    {
        list_free( saved_var );
        result = saved_var = list_new( object_copy( constant_STDERR ) );
    }
    else
#endif
    {
        VARIABLE * v;
        int n;
        if ( ( n = module_get_fixed_var( module, symbol ) ) != -1 )
        {
            if ( DEBUG_VARGET )
                var_dump( symbol, module->fixed_variables[ n ], "get" );
            result = module->fixed_variables[ n ];
        }
        else if ( module->variables && ( v = (VARIABLE *)hash_find(
            module->variables, symbol ) ) )
        {
            if ( DEBUG_VARGET )
                var_dump( v->symbol, v->value, "get" );
            result = v->value;
        }
#ifdef OS_VMS
        else if ( ( module->name && object_equal( module->name, constant_ENVIRON ) )
                  || root_module() == module )
        {
            const char * val = getenv( object_str( symbol ) );
            if ( val )
            {
                struct module_t * environ_module = module;
                char * environ[ 2 ] = { 0 };
                string buf[ 1 ];
                if ( root_module() == module )
                {
                    environ_module = bindmodule( constant_ENVIRON );
                }
                string_copy( buf, object_str( symbol ) );
                string_append( buf, "=" );
                string_append( buf, val );
                environ[ 0 ] = buf->value;
                var_defines( root_module(), environ, 1 );
                var_defines( environ_module, environ, 0 );
                string_free( buf );
                if ( module->variables && ( v = (VARIABLE *)hash_find(
                    module->variables, symbol ) ) )
                {
                    if ( DEBUG_VARGET )
                        var_dump( v->symbol, v->value, "get" );
                    result = v->value;
                }
            }
        }
#endif
    }
    return result;
}
LIST * var_get_and_clear_raw( module_t * module, OBJECT * symbol )
{
    LIST * result = L0;
    VARIABLE * v;
    if ( module->variables && ( v = (VARIABLE *)hash_find( module->variables,
        symbol ) ) )
    {
        result = v->value;
        v->value = L0;
    }
    return result;
}
void var_set( struct module_t * module, OBJECT * symbol, LIST * value, int flag
    )
{
    LIST * * v = var_enter( module, symbol );
    if ( DEBUG_VARSET )
        var_dump( symbol, value, "set" );
    switch ( flag )
    {
    case VAR_SET:
        list_free( *v );
        *v = value;
        break;
    case VAR_APPEND:
        *v = list_append( *v, value );
        break;
    case VAR_DEFAULT:
        if ( list_empty( *v ) )
            *v = value;
        else
            list_free( value );
        break;
    }
}
LIST * var_swap( struct module_t * module, OBJECT * symbol, LIST * value )
{
    LIST * * v = var_enter( module, symbol );
    LIST * oldvalue = *v;
    if ( DEBUG_VARSET )
        var_dump( symbol, value, "set" );
    *v = value;
    return oldvalue;
}
static LIST * * var_enter( struct module_t * module, OBJECT * symbol )
{
    int found;
    VARIABLE * v;
    int n;
    if ( ( n = module_get_fixed_var( module, symbol ) ) != -1 )
        return &module->fixed_variables[ n ];
    if ( !module->variables )
        module->variables = hashinit( sizeof( VARIABLE ), "variables" );
    v = (VARIABLE *)hash_insert( module->variables, symbol, &found );
    if ( !found )
    {
        v->symbol = object_copy( symbol );
        v->value = L0;
    }
    return &v->value;
}
static void var_dump( OBJECT * symbol, LIST * value, const char * what )
{
    out_printf( "%s %s = ", what, object_str( symbol ) );
    list_print( value );
    out_printf( "\n" );
}
static void delete_var_( void * xvar, void * data )
{
    VARIABLE * const v = (VARIABLE *)xvar;
    object_free( v->symbol );
    list_free( v->value );
}
void var_done( struct module_t * module )
{
    list_free( saved_var );
    saved_var = L0;
    hashenumerate( module->variables, delete_var_, 0 );
    hash_free( module->variables );
}
