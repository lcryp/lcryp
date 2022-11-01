#ifndef COMMAND_SW20111118_H
#define COMMAND_SW20111118_H
#include "config.h"
#include "lists.h"
#include "mem.h"
#include "rules.h"
#include "jam_strings.h"
typedef struct _cmd CMD;
typedef struct _cmdlist {
    struct _cmdlist * next;
    union {
        CMD * cmd;
        TARGET * t;
    } impl;
    char iscmd;
} CMDLIST;
CMDLIST * cmdlist_append_cmd( CMDLIST *, CMD * );
CMDLIST * cmdlist_append_target( CMDLIST *, TARGET * );
void cmd_list_free( CMDLIST * );
struct _cmd
{
    CMDLIST * next;
    RULE * rule;
    LIST * shell;
    LOL    args;
    string buf[ 1 ];
    int    noop;
    int    asynccnt;
    targets_ptr lock;
    targets_uptr unlock;
    char   status;
};
CMD * cmd_new
(
    RULE * rule,
    LIST * targets,
    LIST * sources,
    LIST * shell
);
void cmd_release_targets_and_shell( CMD * );
void cmd_free( CMD * );
#define cmd_next( c ) ((c)->next)
#endif
