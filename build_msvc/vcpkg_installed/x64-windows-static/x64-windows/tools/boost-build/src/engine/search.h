#ifndef SEARCH_SW20111118_H
#define SEARCH_SW20111118_H
#include "config.h"
#include "object.h"
#include "timestamp.h"
void set_explicit_binding( OBJECT * target, OBJECT * locate );
OBJECT * search( OBJECT * target, timestamp * const time,
    OBJECT * * another_target, int const file );
void search_done( void );
#endif
