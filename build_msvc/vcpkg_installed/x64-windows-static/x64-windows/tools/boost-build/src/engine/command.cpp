#include "jam.h"
#include "command.h"
#include "lists.h"
#include "rules.h"
#include <assert.h>
CMDLIST * cmdlist_append_cmd( CMDLIST * l, CMD * cmd )
{
    CMDLIST * result = (CMDLIST *)BJAM_MALLOC( sizeof( CMDLIST ) );
    result->iscmd = 1;
    result->next = l;
    result->impl.cmd = cmd;
    return result;
}
CMDLIST * cmdlist_append_target( CMDLIST * l, TARGET * t )
{
    CMDLIST * result = (CMDLIST *)BJAM_MALLOC( sizeof( CMDLIST ) );
    result->iscmd = 0;
    result->next = l;
    result->impl.t = t;
    return result;
}
void cmdlist_free( CMDLIST * l )
{
    while ( l )
    {
        CMDLIST * tmp = l->next;
        BJAM_FREE( l );
        l = tmp;
    }
}
CMD * cmd_new( RULE * rule, LIST * targets, LIST * sources, LIST * shell )
{
    CMD * cmd = b2::jam::make_ptr<CMD>();
    FRAME frame[ 1 ];
    assert( cmd );
    cmd->rule = rule;
    cmd->shell = shell;
    cmd->next = 0;
    cmd->noop = 0;
    cmd->asynccnt = 1;
    cmd->status = 0;
    cmd->lock = NULL;
    cmd->unlock = NULL;
    lol_init( &cmd->args );
    lol_add( &cmd->args, targets );
    lol_add( &cmd->args, sources );
    string_new( cmd->buf );
    frame_init( frame );
    frame->module = rule->module;
    lol_init( frame->args );
    lol_add( frame->args, list_copy( targets ) );
    lol_add( frame->args, list_copy( sources ) );
    function_run_actions( rule->actions->command, frame, cmd->buf );
    frame_free( frame );
    return cmd;
}
void cmd_free( CMD * cmd )
{
    cmdlist_free( cmd->next );
    lol_free( &cmd->args );
    list_free( cmd->shell );
    string_free( cmd->buf );
    b2::jam::free_ptr( cmd );
}
void cmd_release_targets_and_shell( CMD * cmd )
{
    cmd->args.list[ 0 ] = L0;
    cmd->shell = L0;
}
