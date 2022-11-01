#include "config.h"
#include "lists.h"
#include "object.h"
#include "parse.h"
#define YYSTYPE YYSYMBOL
typedef struct _YYSTYPE
{
    int          type;
    OBJECT     * string;
    PARSE      * parse;
    LIST       * list;
    int          number;
    OBJECT     * file;
    int          line;
    char const * keyword;
} YYSTYPE;
extern YYSTYPE yylval;
int yymode( int n );
void yyerror( char const * s );
int yyanyerrors();
void yyfparse( OBJECT * s );
void yyfdone( void );
void yysparse( OBJECT * name, const char * * lines );
int yyline();
int yylex();
int yyparse();
void yyinput_last_read_token( OBJECT * * name, int * line );
#define SCAN_NORMAL  0
#define SCAN_STRING  1
#define SCAN_PUNCT   2
#define SCAN_COND    3
#define SCAN_PARAMS  4
#define SCAN_CALL    5
#define SCAN_CASE    6
#define SCAN_CONDB   7
#define SCAN_ASSIGN  8
#define SCAN_XASSIGN 9
