#include "../mem.h"
#include "../native.h"
#include "../jam_strings.h"
#include "../subst.h"
LIST * regex_split( FRAME * frame, int flags )
{
    LIST * args = lol_get( frame->args, 0 );
    OBJECT * s;
    OBJECT * separator;
    regexp * re;
    const char * pos, * prev;
    LIST * result = L0;
    LISTITER iter = list_begin( args );
    s = list_item( iter );
    separator = list_item( list_next( iter ) );
    re = regex_compile( separator );
    prev = pos = object_str( s );
    while ( regexec( re, pos ) )
    {
        result = list_push_back( result, object_new_range( prev, int32_t(re->startp[ 0 ] - prev) ) );
        prev = re->endp[ 0 ];
        if ( *pos == '\0' )
            break;
        else if ( pos == re->endp[ 0 ] )
            pos++;
        else
            pos = re->endp[ 0 ];
    }
    result = list_push_back( result, object_new( pos ) );
    return result;
}
LIST * regex_replace( FRAME * frame, int flags )
{
    LIST * args = lol_get( frame->args, 0 );
    OBJECT * s;
    OBJECT * match;
    OBJECT * replacement;
    regexp * re;
    const char * pos;
    string buf[ 1 ];
    LIST * result;
    LISTITER iter = list_begin( args );
    s = list_item( iter );
    iter = list_next( iter );
    match = list_item( iter );
    iter = list_next( iter );
    replacement = list_item(iter );
    re = regex_compile( match );
    string_new( buf );
    pos = object_str( s );
    while ( regexec( re, pos ) )
    {
        string_append_range( buf, pos, re->startp[ 0 ] );
        string_append( buf, object_str( replacement ) );
        if ( *pos == '\0' )
            break;
        else if ( pos == re->endp[ 0 ] )
            string_push_back( buf, *pos++ );
        else
            pos = re->endp[ 0 ];
    }
    string_append( buf, pos );
    result = list_new( object_new( buf->value ) );
    string_free( buf );
    return result;
}
LIST * regex_transform( FRAME * frame, int flags )
{
    LIST * const l = lol_get( frame->args, 0 );
    LIST * const pattern = lol_get( frame->args, 1 );
    LIST * const indices_list = lol_get( frame->args, 2 );
    int * indices = 0;
    int size;
    LIST * result = L0;
    if ( !list_empty( indices_list ) )
    {
        int * p;
        LISTITER iter = list_begin( indices_list );
        LISTITER const end = list_end( indices_list );
        size = list_length( indices_list );
        indices = (int *)BJAM_MALLOC( size * sizeof( int ) );
        for ( p = indices; iter != end; iter = list_next( iter ) )
            *p++ = atoi( object_str( list_item( iter ) ) );
    }
    else
    {
        size = 1;
        indices = (int *)BJAM_MALLOC( sizeof( int ) );
        *indices = 1;
    }
    {
        regexp * const re = regex_compile( list_front( pattern ) );
        LISTITER iter = list_begin( l );
        LISTITER const end = list_end( l );
        string buf[ 1 ];
        string_new( buf );
        for ( ; iter != end; iter = list_next( iter ) )
        {
            if ( regexec( re, object_str( list_item( iter ) ) ) )
            {
                int i = 0;
                for ( ; i < size; ++i )
                {
                    int const index = indices[ i ];
                    if ( re->startp[ index ] != re->endp[ index ] )
                    {
                        string_append_range( buf, re->startp[ index ],
                            re->endp[ index ] );
                        result = list_push_back( result, object_new( buf->value
                            ) );
                        string_truncate( buf, 0 );
                    }
                }
            }
        }
        string_free( buf );
    }
    BJAM_FREE( indices );
    return result;
}
void init_regex()
{
    {
        char const * args[] = { "string", "separator", 0  };
        declare_native_rule( "regex", "split", args, regex_split, 1 );
    }
    {
        char const * args[] = { "string", "match", "replacement", 0  };
        declare_native_rule( "regex", "replace", args, regex_replace, 1 );
    }
    {
        char const * args[] = { "list", "*", ":", "pattern", ":", "indices", "*", 0 };
        declare_native_rule( "regex", "transform", args, regex_transform, 2 );
    }
}
