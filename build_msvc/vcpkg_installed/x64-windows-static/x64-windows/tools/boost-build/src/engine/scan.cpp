#include "jam.h"
#include "scan.h"
#include "output.h"
#include "constants.h"
#include "jamgram.hpp"
struct keyword
{
    const char * word;
    int    type;
} keywords[] =
{
#include "jamgramtab.h"
    { 0, 0 }
};
typedef struct include include;
struct include
{
    include   * next;
    char      * string;
    char    * * strings;
    LISTITER    pos;
    LIST      * list;
    FILE      * file;
    OBJECT    * fname;
    int         line;
    char        buf[ 512 ];
};
static include * incp = 0;
static int scanmode = SCAN_NORMAL;
static int anyerrors = 0;
static char * symdump( YYSTYPE * );
#define BIGGEST_TOKEN 10240
int yymode( int n )
{
    int result = scanmode;
    scanmode = n;
    return result;
}
void yyerror( char const * s )
{
    out_printf( "%s:%d: %s at %s\n", object_str( yylval.file ), yylval.line, s,
            symdump( &yylval ) );
    ++anyerrors;
}
int yyanyerrors()
{
    return anyerrors != 0;
}
void yyfparse( OBJECT * s )
{
    include * i = (include *)BJAM_MALLOC( sizeof( *i ) );
    i->string = (char*)"";
    i->strings = 0;
    i->file = 0;
    i->fname = object_copy( s );
    i->line = 0;
    i->next = incp;
    incp = i;
}
void yysparse( OBJECT * name, const char * * lines )
{
    yyfparse( name );
    incp->strings = (char * *)lines;
}
void yyfdone( void )
{
    include * const i = incp;
    incp = i->next;
    if(i->file && (i->file != stdin))
        fclose(i->file);
    object_free(i->fname);
    BJAM_FREE((char *)i);
}
int yyline()
{
    include * const i = incp;
    if ( !incp )
        return EOF;
    if ( *i->string )
        return *i->string++;
    if ( i->strings )
    {
        if ( *i->strings )
        {
            ++i->line;
            i->string = *(i->strings++);
            return *i->string++;
        }
    }
    else
    {
        if ( !i->file )
        {
            FILE * f = stdin;
            if ( strcmp( object_str( i->fname ), "-" ) && !( f = fopen( object_str( i->fname ), "r" ) ) )
                errno_puts( object_str( i->fname ) );
            i->file = f;
        }
        if ( i->file && fgets( i->buf, sizeof( i->buf ), i->file ) )
        {
            ++i->line;
            i->string = i->buf;
            return *i->string++;
        }
    }
    return EOF;
}
int yypeek()
{
    if ( *incp->string )
    {
        return *incp->string;
    }
    else if ( incp->strings )
    {
        if ( *incp->strings )
            return **incp->strings;
    }
    else if ( incp->file )
    {
        int ch = fgetc( incp->file );
        if ( ch != EOF )
            ungetc( ch, incp->file );
        return ch;
    }
    return EOF;
}
#define yychar() ( *incp->string ? *incp->string++ : yyline() )
#define yyprev() ( incp->string-- )
static int use_new_scanner = 0;
#define yystartkeyword() if(use_new_scanner) break; else token_warning()
#define yyendkeyword() if(use_new_scanner) break; else if ( 1 ) { expect_whitespace = 1; continue; } else (void)0
void do_token_warning()
{
    out_printf( "%s:%d: %s %s\n", object_str( yylval.file ), yylval.line, "Unescaped special character in",
            symdump( &yylval ) );
}
#define token_warning() has_token_warning = 1
int yylex()
{
    int c;
    char buf[ BIGGEST_TOKEN ];
    char * b = buf;
    if ( !incp )
        goto eof;
    c = yychar();
    if ( scanmode == SCAN_STRING )
    {
        int nest = 1;
        while ( ( c != EOF ) && ( b < buf + sizeof( buf ) ) )
        {
            if ( c == '{' )
                ++nest;
            if ( ( c == '}' ) && !--nest )
                break;
            *b++ = c;
            c = yychar();
            if ( ( c == '\n' ) && ( b[ -1 ] == '\r' ) )
                --b;
        }
        if ( c != EOF )
            yyprev();
        if ( b == buf + sizeof( buf ) )
        {
            yyerror( "action block too big" );
            goto eof;
        }
        if ( nest )
        {
            yyerror( "unmatched {} in action block" );
            goto eof;
        }
        *b = 0;
        yylval.type = STRING;
        yylval.string = object_new( buf );
        yylval.file = incp->fname;
        yylval.line = incp->line;
    }
    else
    {
        char * b = buf;
        struct keyword * k;
        int inquote = 0;
        int notkeyword;
        int hastoken = 0;
        int hasquote = 0;
        int ingrist = 0;
        int invarexpand = 0;
        int expect_whitespace = 0;
        int has_token_warning = 0;
        for ( ; ; )
        {
            while ( ( c != EOF ) && isspace( c ) )
                c = yychar();
            if ( c != '#' )
                break;
            c = yychar();
            if ( ( c != EOF ) && c == '|' )
            {
                int c0 = yychar();
                int c1 = yychar();
                while ( ! ( c0 == '|' && c1 == '#' ) && ( c0 != EOF && c1 != EOF ) )
                {
                    c0 = c1;
                    c1 = yychar();
                }
                c = yychar();
            }
            else
            {
                while ( ( c != EOF ) && ( c != '\n' ) ) c = yychar();
            }
        }
        if ( c == EOF )
            goto eof;
        yylval.file = incp->fname;
        yylval.line = incp->line;
        notkeyword = c == '$';
        while
        (
            ( c != EOF ) &&
            ( b < buf + sizeof( buf ) ) &&
            ( inquote || invarexpand || !isspace( c ) )
        )
        {
            if ( expect_whitespace || ( isspace( c ) && ! inquote ) )
            {
                token_warning();
                expect_whitespace = 0;
            }
            if ( !inquote && !invarexpand )
            {
                if ( scanmode == SCAN_COND || scanmode == SCAN_CONDB )
                {
                    if ( hastoken && ( c == '=' || c == '<' || c == '>' || c == '!' || c == '(' || c == ')' || c == '&' || c == '|' ) )
                    {
                        if ( ! ( scanmode == SCAN_CONDB && ingrist == 1 && c == '>' ) )
                        {
                            yystartkeyword();
                        }
                    }
                    else if ( c == '=' || c == '(' || c == ')' )
                    {
                        *b++ = c;
                        c = yychar();
                        yyendkeyword();
                    }
                    else if ( c == '!' || ( scanmode == SCAN_COND && ( c == '<' || c == '>' ) ) )
                    {
                        *b++ = c;
                        if ( ( c = yychar() ) == '=' )
                        {
                            *b++ = c;
                            c = yychar();
                        }
                        yyendkeyword();
                    }
                    else if ( c == '&' || c == '|' )
                    {
                        *b++ = c;
                        if ( yychar() == c )
                        {
                            *b++ = c;
                            c = yychar();
                        }
                        yyendkeyword();
                    }
                }
                else if ( scanmode == SCAN_PARAMS )
                {
                    if ( c == '*' || c == '+' || c == '?' || c == '(' || c == ')' )
                    {
                        if ( !hastoken )
                        {
                            *b++ = c;
                            c = yychar();
                            yyendkeyword();
                        }
                        else
                        {
                            yystartkeyword();
                        }
                    }
                }
                else if ( scanmode == SCAN_XASSIGN && ! hastoken )
                {
                    if ( c == '=' )
                    {
                        *b++ = c;
                        c = yychar();
                        yyendkeyword();
                    }
                    else if ( c == '+' || c == '?' )
                    {
                        if ( yypeek() == '=' )
                        {
                            *b++ = c;
                            *b++ = yychar();
                            c = yychar();
                            yyendkeyword();
                        }
                    }
                }
                else if ( scanmode == SCAN_NORMAL || scanmode == SCAN_ASSIGN )
                {
                    if ( c == '=' )
                    {
                        if ( !hastoken )
                        {
                            *b++ = c;
                            c = yychar();
                            yyendkeyword();
                        }
                        else
                        {
                            yystartkeyword();
                        }
                    }
                    else if ( c == '+' || c == '?' )
                    {
                        if ( yypeek() == '=' )
                        {
                            if ( hastoken )
                            {
                                yystartkeyword();
                            }
                            else
                            {
                                *b++ = c;
                                *b++ = yychar();
                                c = yychar();
                                yyendkeyword();
                            }
                        }
                    }
                }
                if ( scanmode != SCAN_CASE && ( c == ';' || c == '{' || c == '}' ||
                    ( scanmode != SCAN_PARAMS && ( c == '[' || c == ']' ) ) ) )
                {
                    if ( ! hastoken )
                    {
                        *b++ = c;
                        c = yychar();
                        yyendkeyword();
                    }
                    else
                    {
                        yystartkeyword();
                    }
                }
                else if ( c == ':' )
                {
                    if ( ! hastoken )
                    {
                        *b++ = c;
                        c = yychar();
                        yyendkeyword();
                        break;
                    }
                    else if ( hasquote )
                    {
                        yystartkeyword();
                    }
                    else if ( ingrist == 0 )
                    {
                        int next = yychar();
                        int is_win_path = 0;
                        int is_conditional = 0;
                        if ( next == '\\' )
                        {
                            if( yypeek() == '\\' )
                            {
                                is_win_path = 1;
                            }
                        }
                        else if ( next == '/' )
                        {
                            is_win_path = 1;
                        }
                        yyprev();
                        if ( is_win_path )
                        {
                            if ( b > buf && isalpha( b[ -1 ] ) && ( b == buf + 1 || b[ -2 ] == '>' ) )
                            {
                                is_win_path = 1;
                            }
                            else
                            {
                                is_win_path = 0;
                            }
                        }
                        if ( next == '<' )
                        {
                            if ( ( (b > buf) && (buf[ 0 ] == '<') ) ||
                                ( (b > (buf + 1)) && (buf[ 0 ] == '!') && (buf[ 1 ] == '<') ))
                            {
                                is_conditional = 1;
                            }
                        }
                        if ( !is_conditional && !is_win_path )
                        {
                            yystartkeyword();
                        }
                    }
                }
            }
            hastoken = 1;
            if ( c == '"' )
            {
                inquote = !inquote;
                hasquote = 1;
                notkeyword = 1;
            }
            else if ( c != '\\' )
            {
                if ( !invarexpand && c == '<' )
                {
                    if ( ingrist == 0 ) ingrist = 1;
                    else ingrist = -1;
                }
                else if ( !invarexpand && c == '>' )
                {
                    if ( ingrist == 1 ) ingrist = 0;
                    else ingrist = -1;
                }
                else if ( c == '$' )
                {
                    if ( ( c = yychar() ) == EOF )
                    {
                        *b++ = '$';
                        break;
                    }
                    else if ( c == '(' )
                    {
                        *b++ = '$';
                        c = '(';
                        ++invarexpand;
                    }
                    else
                    {
                        c = '$';
                        yyprev();
                    }
                }
                else if ( c == '@' )
                {
                    if ( ( c = yychar() ) == EOF )
                    {
                        *b++ = '@';
                        break;
                    }
                    else if ( c == '(' )
                    {
                        *b++ = '@';
                        c = '(';
                        ++invarexpand;
                    }
                    else
                    {
                        c = '@';
                        yyprev();
                    }
                }
                else if ( invarexpand && c == '(' )
                {
                    ++invarexpand;
                }
                else if ( invarexpand && c == ')' )
                {
                    --invarexpand;
                }
                *b++ = c;
            }
            else if ( ( c = yychar() ) != EOF )
            {
                if (c == 'n')
                    c = '\n';
                else if (c == 'r')
                    c = '\r';
                else if (c == 't')
                    c = '\t';
                *b++ = c;
                notkeyword = 1;
            }
            else
            {
                break;
            }
            c = yychar();
        }
        if ( scanmode == SCAN_CONDB )
            scanmode = SCAN_COND;
        if ( b == buf + sizeof( buf ) )
        {
            yyerror( "string too big" );
            goto eof;
        }
        if ( inquote )
        {
            yyerror( "unmatched \" in string" );
            goto eof;
        }
        if ( c != EOF )
            yyprev();
        *b = 0;
        yylval.type = ARG;
        if ( !notkeyword && !( isalpha( *buf ) && ( scanmode == SCAN_PUNCT || scanmode == SCAN_PARAMS || scanmode == SCAN_ASSIGN ) ) )
            for ( k = keywords; k->word; ++k )
                if ( ( *buf == *k->word ) && !strcmp( k->word, buf ) )
                {
                    yylval.type = k->type;
                    yylval.keyword = k->word;
                    break;
                }
        if ( yylval.type == ARG )
            yylval.string = object_new( buf );
        if ( scanmode == SCAN_NORMAL && yylval.type == ARG )
            scanmode = SCAN_XASSIGN;
        if ( has_token_warning )
            do_token_warning();
    }
    if ( DEBUG_SCAN )
        out_printf( "scan %s\n", symdump( &yylval ) );
    return yylval.type;
eof:
    yylval.type = EOF;
    return yylval.type;
}
static char * symdump( YYSTYPE * s )
{
    static char buf[ BIGGEST_TOKEN + 20 ];
    switch ( s->type )
    {
        case EOF   : sprintf( buf, "EOF"                                        ); break;
        case 0     : sprintf( buf, "unknown symbol %s", object_str( s->string ) ); break;
        case ARG   : sprintf( buf, "argument %s"      , object_str( s->string ) ); break;
        case STRING: sprintf( buf, "string \"%s\""    , object_str( s->string ) ); break;
        default    : sprintf( buf, "keyword %s"       , s->keyword              ); break;
    }
    return buf;
}
void yyinput_last_read_token( OBJECT * * name, int * line )
{
    *name = yylval.file;
    *line = yylval.line;
}
