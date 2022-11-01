#ifndef NATIVE_H_VP_2003_12_09
#define NATIVE_H_VP_2003_12_09
#include "config.h"
#include "function.h"
#include "frames.h"
#include "lists.h"
#include "object.h"
typedef struct native_rule_t
{
    OBJECT * name;
    FUNCTION * procedure;
    int32_t version;
} native_rule_t;
void declare_native_rule( char const * module, char const * rule,
    char const * * args, LIST * (*f)( FRAME *, int32_t ), int32_t version );
#endif
