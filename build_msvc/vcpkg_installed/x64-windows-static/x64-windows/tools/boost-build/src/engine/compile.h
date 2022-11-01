#ifndef COMPILE_DWA20011022_H
#define COMPILE_DWA20011022_H
#include "config.h"
#include "frames.h"
#include "lists.h"
#include "object.h"
#include "rules.h"
void compile_builtins();
LIST * evaluate_rule( RULE * rule, OBJECT * rulename, FRAME * );
LIST * call_rule( OBJECT * rulename, FRAME * caller_frame, ... );
#define ASSIGN_SET      0x00
#define ASSIGN_APPEND   0x01
#define ASSIGN_DEFAULT  0x02
#define EXEC_UPDATED    0x01
#define EXEC_TOGETHER   0x02
#define EXEC_IGNORE     0x04
#define EXEC_QUIETLY    0x08
#define EXEC_PIECEMEAL  0x10
#define EXEC_EXISTING   0x20
#define EXPR_NOT     0
#define EXPR_AND     1
#define EXPR_OR      2
#define EXPR_EXISTS  3
#define EXPR_EQUALS  4
#define EXPR_NOTEQ   5
#define EXPR_LESS    6
#define EXPR_LESSEQ  7
#define EXPR_MORE    8
#define EXPR_MOREEQ  9
#define EXPR_IN      10
#endif
