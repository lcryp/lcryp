#include "jam.h"
#include "search.h"
#include "compile.h"
#include "filesys.h"
#include "hash.h"
#include "lists.h"
#include "object.h"
#include "pathsys.h"
#include "jam_strings.h"
#include "timestamp.h"
#include "variable.h"
#include "output.h"
#include <string.h>
typedef struct _binding
{
    OBJECT * binding;
    OBJECT * target;
} BINDING;
static struct hash * explicit_bindings = 0;
void call_bind_rule( OBJECT * target_, OBJECT * boundname_ )
{
    LIST * const bind_rule = var_get( root_module(), constant_BINDRULE );
    if ( !list_empty( bind_rule ) )
    {
        OBJECT * target = object_copy( target_ );
        OBJECT * boundname = object_copy( boundname_ );
        if ( boundname && target )
        {
            FRAME frame[ 1 ];
            frame_init( frame );
            lol_add( frame->args, list_new( target ) );
            lol_add( frame->args, list_new( boundname ) );
            if ( lol_get( frame->args, 1 ) )
            {
                OBJECT * rulename = list_front( bind_rule );
                list_free( evaluate_rule( bindrule( rulename, root_module() ), rulename, frame ) );
            }
            frame_free( frame );
        }
        else
        {
            if ( boundname )
                object_free( boundname );
            if ( target )
                object_free( target );
        }
    }
}
void set_explicit_binding( OBJECT * target, OBJECT * locate )
{
    OBJECT * boundname;
    OBJECT * key;
    PATHNAME f[ 1 ];
    string buf[ 1 ];
    int found;
    BINDING * ba;
    if ( !explicit_bindings )
        explicit_bindings = hashinit( sizeof( BINDING ), "explicitly specified "
            "locations" );
    string_new( buf );
    path_parse( object_str( target ), f );
    f->f_grist.ptr = 0;
    f->f_grist.len = 0;
    f->f_root.ptr = object_str( locate );
    f->f_root.len = int32_t(strlen( object_str( locate ) ));
    path_build( f, buf );
    boundname = object_new( buf->value );
    if ( DEBUG_SEARCH )
        out_printf( "explicit locate %s: %s\n", object_str( target ), buf->value );
    string_free( buf );
    key = path_as_key( boundname );
    object_free( boundname );
    ba = (BINDING *)hash_insert( explicit_bindings, key, &found );
    if ( !found )
    {
        ba->binding = key;
        ba->target = target;
    }
    else
        object_free( key );
}
OBJECT * search( OBJECT * target, timestamp * const time,
    OBJECT * * another_target, int const file )
{
    PATHNAME f[ 1 ];
    LIST * varlist;
    string buf[ 1 ];
    int found = 0;
    OBJECT * boundname = 0;
    if ( another_target )
        *another_target = 0;
    if ( !explicit_bindings )
        explicit_bindings = hashinit( sizeof( BINDING ), "explicitly specified "
            "locations" );
    string_new( buf );
    path_parse( object_str( target ), f );
    f->f_grist.ptr = 0;
    f->f_grist.len = 0;
    varlist = var_get( root_module(), constant_LOCATE );
    if ( !list_empty( varlist ) )
    {
        OBJECT * key;
        f->f_root.ptr = object_str( list_front( varlist ) );
        f->f_root.len = int32_t(strlen( object_str( list_front( varlist ) ) ));
        path_build( f, buf );
        if ( DEBUG_SEARCH )
            out_printf( "locate %s: %s\n", object_str( target ), buf->value );
        key = object_new( buf->value );
        timestamp_from_path( time, key );
        object_free( key );
        found = 1;
    }
    else if ( varlist = var_get( root_module(), constant_SEARCH ),
        !list_empty( varlist ) )
    {
        LISTITER iter = list_begin( varlist );
        LISTITER const end = list_end( varlist );
        for ( ; iter != end; iter = list_next( iter ) )
        {
            BINDING * ba;
            file_info_t * ff;
            OBJECT * key;
            OBJECT * test_path;
            f->f_root.ptr = object_str( list_item( iter ) );
            f->f_root.len = int32_t(strlen( object_str( list_item( iter ) ) ));
            string_truncate( buf, 0 );
            path_build( f, buf );
            if ( DEBUG_SEARCH )
                out_printf( "search %s: %s\n", object_str( target ), buf->value );
            test_path = object_new( buf->value );
            key = path_as_key( test_path );
            object_free( test_path );
            ff = file_query( key );
            timestamp_from_path( time, key );
            if ( ( ba = (BINDING *)hash_find( explicit_bindings, key ) ) )
            {
                if ( DEBUG_SEARCH )
                    out_printf(" search %s: found explicitly located target %s\n",
                        object_str( target ), object_str( ba->target ) );
                if ( another_target )
                    *another_target = ba->target;
                found = 1;
                object_free( key );
                break;
            }
            else if ( ff )
            {
                if ( !file || ff->is_file )
                {
                    found = 1;
                    object_free( key );
                    break;
                }
            }
            object_free( key );
        }
    }
    if ( !found )
    {
        OBJECT * key;
        f->f_root.ptr = 0;
        f->f_root.len = 0;
        string_truncate( buf, 0 );
        path_build( f, buf );
        if ( DEBUG_SEARCH )
            out_printf( "search %s: %s\n", object_str( target ), buf->value );
        key = object_new( buf->value );
        timestamp_from_path( time, key );
        object_free( key );
    }
    boundname = object_new( buf->value );
    string_free( buf );
    call_bind_rule( target, boundname );
    return boundname;
}
static void free_binding( void * xbinding, void * data )
{
    object_free( ( (BINDING *)xbinding )->binding );
}
void search_done( void )
{
    if ( explicit_bindings )
    {
        hashenumerate( explicit_bindings, free_binding, 0 );
        hashdone( explicit_bindings );
    }
}
