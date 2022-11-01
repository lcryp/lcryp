#include "jam.h"
#include "lists.h"
#include "parse.h"
#include "scan.h"
#include "object.h"
#include "modules.h"
#include "frames.h"
#include "function.h"
#include "mem.h"
#include "startup.h"
#include <set>
static PARSE * yypsave;
static std::set<PARSE*> parse_mem;
struct parse_ptr
{
    parse_ptr() : ptr( yypsave ) {}
    ~parse_ptr() { if ( ptr ) parse_free( ptr ); }
    operator PARSE*() const { return ptr; }
    private:
    PARSE * ptr = nullptr;
};
static void parse_impl( FRAME * frame )
{
    for ( ; ; )
    {
        yypsave = nullptr;
        int yy_result = yyparse();
        parse_ptr p;
        if ( yy_result || !p )
            break;
        auto func = b2::jam::make_unique_bare_jptr( function_compile( p ), function_free );
        list_free( function_run( func.get(), frame ) );
    }
    yyfdone();
}
void parse_file( OBJECT * f, FRAME * frame )
{
    yyfparse( f );
    parse_impl( frame );
}
void parse_string( OBJECT * name, const char * * lines, FRAME * frame )
{
    yysparse( name, lines );
    parse_impl( frame );
}
void parse_save( PARSE * p )
{
    yypsave = p;
}
PARSE * parse_make(
    int      type,
    PARSE *& left,
    PARSE *& right,
    PARSE *& third,
    OBJECT * string,
    OBJECT * string1,
    int      num )
{
    PARSE * p = (PARSE *)BJAM_MALLOC( sizeof( PARSE ) );
    p->type = type;
    p->left = left; left = nullptr;
    p->right = right; right = nullptr;
    p->third = third; third = nullptr;
    p->string = string;
    p->string1 = string1;
    p->num = num;
    p->rulename = 0;
    if ( p->left )
    {
        p->file = object_copy( p->left->file );
        p->line = p->left->line;
    }
    else
    {
        yyinput_last_read_token( &p->file, &p->line );
        p->file = object_copy( p->file );
    }
    parse_mem.insert( p );
    return p;
}
void parse_free( PARSE * & p )
{
    if ( parse_mem.count( p ) == 0 ) return;
    parse_mem.erase( p );
    if ( p->string )
        object_free( p->string );
    if ( p->string1 )
        object_free( p->string1 );
    if ( p->left )
        parse_free( p->left );
    if ( p->right )
        parse_free( p->right );
    if ( p->third )
        parse_free( p->third );
    if ( p->rulename )
        object_free( p->rulename );
    if ( p->file )
        object_free( p->file );
    BJAM_FREE( (char *)p );
    p = nullptr;
}
void parse_done()
{
    while ( parse_mem.size() > 0 )
    {
        PARSE * p = *parse_mem.begin();
        parse_free( p );
    }
}
