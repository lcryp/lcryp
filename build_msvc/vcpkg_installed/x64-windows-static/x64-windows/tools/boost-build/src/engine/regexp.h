#ifndef REGEXP_DWA20011023_H
#define REGEXP_DWA20011023_H
#include "config.h"
#define NSUBEXP  10
typedef struct regexp {
    char const * startp[ NSUBEXP ];
    char const * endp[ NSUBEXP ];
    char regstart;
    char reganch;
    char * regmust;
    int32_t regmlen;
    char program[ 1 ];
} regexp;
regexp * regcomp( char const * exp );
int32_t regexec( regexp * prog, char const * string );
void regerror( char const * s );
#define MAGIC  0234
#endif
