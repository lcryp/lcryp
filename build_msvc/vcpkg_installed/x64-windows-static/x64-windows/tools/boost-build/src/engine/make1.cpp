#include "jam.h"
#include "make.h"
#include "command.h"
#include "compile.h"
#include "execcmd.h"
#include "headers.h"
#include "lists.h"
#include "object.h"
#include "output.h"
#include "parse.h"
#include "rules.h"
#include "search.h"
#include "variable.h"
#include "output.h"
#include "startup.h"
#include <assert.h>
#include <stdlib.h>
#if !defined( NT ) || defined( __GNUC__ )
    #include <unistd.h>
#endif
static CMD      * make1cmds      ( TARGET * );
static LIST     * make1list      ( LIST *, const targets_uptr &, int32_t flags );
static SETTINGS * make1settings  ( struct module_t *, LIST * vars );
static void       make1bind      ( TARGET * );
static void       push_cmds( CMDLIST * cmds, int32_t status );
static int32_t    cmd_sem_lock( TARGET * t );
static void       cmd_sem_unlock( TARGET * t );
static bool targets_contains( const targets_uptr & l, TARGET * t );
static bool targets_equal( const targets_uptr & l1, const targets_uptr & l2 );
static struct
{
    int32_t failed;
    int32_t skipped;
    int32_t total;
    int32_t made;
} counts[ 1 ];
#define T_STATE_MAKE1A  0
#define T_STATE_MAKE1B  1
#define T_STATE_MAKE1C  2
typedef struct _state state;
struct _state
{
    state  * prev;
    TARGET * t;
    TARGET * parent;
    int32_t  curstate;
};
static void make1a( state * const );
static void make1b( state * const );
static void make1c( state const * const );
static void make1c_closure( void * const closure, int32_t status,
    timing_info const * const, char const * const cmd_stdout,
    char const * const cmd_stderr, int32_t const cmd_exit_reason );
typedef struct _stack
{
    state * stack;
} stack;
static stack state_stack = { NULL };
static state * state_freelist = NULL;
static int32_t cmdsrunning;
static state * alloc_state()
{
    if ( state_freelist )
    {
        state * const pState = state_freelist;
        state_freelist = pState->prev;
        memset( pState, 0, sizeof( state ) );
        return pState;
    }
    return (state *)BJAM_MALLOC( sizeof( state ) );
}
static void free_state( state * const pState )
{
    pState->prev = state_freelist;
    state_freelist = pState;
}
static void clear_state_freelist()
{
    while ( state_freelist )
    {
        state * const pState = state_freelist;
        state_freelist = state_freelist->prev;
        BJAM_FREE( pState );
    }
}
static state * current_state( stack * const pStack )
{
    return pStack->stack;
}
static void pop_state( stack * const pStack )
{
    if ( pStack->stack )
    {
        state * const pState = pStack->stack->prev;
        free_state( pStack->stack );
        pStack->stack = pState;
    }
}
static state * push_state( stack * const pStack, TARGET * const t,
    TARGET * const parent, int32_t const curstate )
{
    state * const pState = alloc_state();
    pState->t = t;
    pState->parent = parent;
    pState->prev = pStack->stack;
    pState->curstate = curstate;
    return pStack->stack = pState;
}
static void push_stack_on_stack( stack * const pDest, stack * const pSrc )
{
    while ( pSrc->stack )
    {
        state * const pState = pSrc->stack;
        pSrc->stack = pState->prev;
        pState->prev = pDest->stack;
        pDest->stack = pState;
    }
}
static int32_t intr = 0;
static int32_t quit = 0;
int32_t make1( LIST * targets )
{
    state * pState;
    int32_t status = 0;
    memset( (char *)counts, 0, sizeof( *counts ) );
    {
        LISTITER iter, end;
        stack temp_stack = { NULL };
        for ( iter = list_begin( targets ), end = list_end( targets );
              iter != end; iter = list_next( iter ) )
            push_state( &temp_stack, bindtarget( list_item( iter ) ), NULL, T_STATE_MAKE1A );
        push_stack_on_stack( &state_stack, &temp_stack );
    }
    quit = 0;
    while ( 1 )
    {
        while ( ( pState = current_state( &state_stack ) ) )
        {
            if ( quit )
                pop_state( &state_stack );
            switch ( pState->curstate )
            {
                case T_STATE_MAKE1A: make1a( pState ); break;
                case T_STATE_MAKE1B: make1b( pState ); break;
                case T_STATE_MAKE1C: make1c( pState ); break;
                default:
                    assert( !"make1(): Invalid state detected." );
            }
        }
        if ( !cmdsrunning )
            break;
        exec_wait();
    }
    clear_state_freelist();
    if ( counts->failed )
        out_printf( "...failed updating %d target%s...\n", counts->failed,
            counts->failed > 1 ? "s" : "" );
    if ( DEBUG_MAKE && counts->skipped )
        out_printf( "...skipped %d target%s...\n", counts->skipped,
            counts->skipped > 1 ? "s" : "" );
    if ( DEBUG_MAKE && counts->made )
        out_printf( "...updated %d target%s...\n", counts->made,
            counts->made > 1 ? "s" : "" );
    if ( intr )
        b2::clean_exit( EXITBAD );
    {
        LISTITER iter, end;
        for ( iter = list_begin( targets ), end = list_end( targets );
              iter != end; iter = list_next( iter ) )
        {
            TARGET * t = bindtarget( list_item( iter ) );
            if (t->progress == T_MAKE_DONE)
            {
                if (t->status != EXEC_CMD_OK)
                    status = 1;
            }
            else if ( ! ( t->progress == T_MAKE_NOEXEC_DONE && globs.noexec ) )
            {
                status = 1;
            }
        }
    }
    return status;
}
static void make1a( state * const pState )
{
    TARGET * t = pState->t;
    TARGET * const scc_root = target_scc( t );
    if ( !pState->parent || target_scc( pState->parent ) != scc_root )
        pState->t = t = scc_root;
    if ( pState->parent && t->progress <= T_MAKE_RUNNING )
    {
        TARGET * const parent_scc = target_scc( pState->parent );
        if ( t != parent_scc )
        {
            targetentry( t->parents, parent_scc );
            ++parent_scc->asynccnt;
        }
    }
    if ( !globs.noexec && t->progress == T_MAKE_NOEXEC_DONE )
        t->progress = T_MAKE_INIT;
    if ( t->progress != T_MAKE_INIT )
    {
        pop_state( &state_stack );
        return;
    }
    t->progress = T_MAKE_ONSTACK;
    t->asynccnt = 1;
    {
        stack temp_stack = { NULL };
        targets_ptr c;
        for ( c = t->depends.get(); c && !quit; c = c->next.get() )
            push_state( &temp_stack, c->target, t, T_STATE_MAKE1A );
        push_stack_on_stack( &state_stack, &temp_stack );
    }
    t->progress = T_MAKE_ACTIVE;
    pState->curstate = T_STATE_MAKE1B;
}
static void make1b( state * const pState )
{
    TARGET * const t = pState->t;
    TARGET * failed = 0;
    char const * failed_name = "dependencies";
    pop_state( &state_stack );
    if ( --t->asynccnt )
    {
        return;
    }
    if ( !globs.noexec )
    {
        targets_ptr c;
        for ( c = t->depends.get(); c; c = c->next.get() )
            if ( c->target->status > t->status && !( c->target->flags &
                T_FLAG_NOCARE ) )
            {
                failed = c->target;
                t->status = c->target->status;
            }
    }
    if ( failed )
        failed_name = failed->flags & T_FLAG_INTERNAL
            ? failed->failed
            : object_str( failed->name );
    t->failed = failed_name;
    if ( ( t->status == EXEC_CMD_FAIL ) && t->actions )
    {
        ++counts->skipped;
        if ( ( t->flags & ( T_FLAG_RMOLD | T_FLAG_NOTFILE ) ) == T_FLAG_RMOLD )
        {
            if ( !unlink( object_str( t->boundname ) ) )
                out_printf( "...removing outdated %s\n", object_str( t->boundname )
                    );
        }
        else
            out_printf( "...skipped %s for lack of %s...\n", object_str( t->name ),
                failed_name );
    }
    if ( t->status == EXEC_CMD_OK )
        switch ( t->fate )
        {
        case T_FATE_STABLE:
        case T_FATE_NEWER:
            break;
        case T_FATE_CANTFIND:
        case T_FATE_CANTMAKE:
            t->status = EXEC_CMD_FAIL;
            break;
        case T_FATE_ISTMP:
            if ( DEBUG_MAKE )
                out_printf( "...using %s...\n", object_str( t->name ) );
            break;
        case T_FATE_TOUCHED:
        case T_FATE_MISSING:
        case T_FATE_NEEDTMP:
        case T_FATE_OUTDATED:
        case T_FATE_UPDATE:
        case T_FATE_REBUILD:
            if ( t->actions )
            {
                ++counts->total;
                if ( DEBUG_MAKE && !( counts->total % 100 ) )
                    out_printf( "...on %dth target...\n", counts->total );
                t->cmds = (char *)make1cmds( t );
                t->progress = T_MAKE_RUNNING;
            }
            break;
        default:
            err_printf( "ERROR: %s has bad fate %d", object_str( t->name ),
                t->fate );
            b2::clean_exit( b2::exit_result::failure );
        }
    if ( t->cmds == NULL || --( ( CMD * )t->cmds )->asynccnt == 0 )
        push_state( &state_stack, t, NULL, T_STATE_MAKE1C );
    else if ( DEBUG_EXECCMD )
    {
        CMD * cmd = ( CMD * )t->cmds;
        out_printf( "Delaying %s %s: %d targets not ready\n", object_str( cmd->rule->name ), object_str( t->boundname ), cmd->asynccnt );
    }
}
static void make1c( state const * const pState )
{
    TARGET * const t = pState->t;
    CMD * const cmd = (CMD *)t->cmds;
    int32_t exec_flags = 0;
    if ( cmd )
    {
        pop_state( &state_stack );
        if ( cmd->status != EXEC_CMD_OK )
        {
            t->cmds = NULL;
            push_cmds( cmd->next, cmd->status );
            cmd_free( cmd );
            return;
        }
#ifdef OPT_SEMAPHORE
        if ( ! cmd_sem_lock( t ) )
        {
            return;
        }
#endif
        ++cmdsrunning;
        if ( ( globs.jobs == 1 ) && ( DEBUG_MAKEQ ||
            ( DEBUG_MAKE && !( cmd->rule->actions->flags & RULE_QUIETLY ) ) ) )
        {
            OBJECT * action  = cmd->rule->name;
            OBJECT * target = list_front( lol_get( (LOL *)&cmd->args, 0 ) );
            out_printf( "%s %s\n", object_str( action ), object_str( target ) );
            if ( DEBUG_EXEC )
            {
                out_puts( cmd->buf->value );
                out_putc( '\n' );
            }
            if ( ! globs.noexec && ! cmd->noop )
            {
                out_flush();
                err_flush();
            }
        }
        else
        {
            exec_flags |= EXEC_CMD_QUIET;
        }
        if ( globs.noexec || cmd->noop )
        {
            timing_info time_info = { 0 };
            timestamp_current( &time_info.start );
            timestamp_copy( &time_info.end, &time_info.start );
            make1c_closure( t, EXEC_CMD_OK, &time_info, "", "", EXIT_OK );
        }
        else
        {
            exec_cmd( cmd->buf, exec_flags, make1c_closure, t, cmd->shell );
            assert( 0 < globs.jobs );
            while ( cmdsrunning >= globs.jobs )
                exec_wait();
        }
    }
    else
    {
        if ( t->progress == T_MAKE_RUNNING )
        {
            if ( t->flags & T_FLAG_FAIL_EXPECTED && !globs.noexec )
            {
                switch ( t->status )
                {
                    case EXEC_CMD_FAIL: t->status = EXEC_CMD_OK; break;
                    case EXEC_CMD_OK: t->status = EXEC_CMD_FAIL; break;
                }
                if ( t->status == EXEC_CMD_FAIL )
                {
                    out_printf( "...failed %s ", object_str( t->actions->action->rule->name ) );
                    out_printf( "%s", object_str( t->boundname ) );
                    out_printf( "...\n" );
                }
                if ( t->status == EXEC_CMD_FAIL && globs.quitquick )
                    ++quit;
                if ( !( t->flags & ( T_FLAG_PRECIOUS | T_FLAG_NOTFILE ) ) &&
                    !unlink( object_str( t->boundname ) ) )
                    out_printf( "...removing %s\n", object_str( t->boundname ) );
            }
            switch ( t->status )
            {
                case EXEC_CMD_OK: ++counts->made; break;
                case EXEC_CMD_FAIL: ++counts->failed; break;
            }
        }
        {
            targets_ptr c;
            stack temp_stack = { NULL };
            TARGET * additional_includes = NULL;
            t->progress = globs.noexec ? T_MAKE_NOEXEC_DONE : T_MAKE_DONE;
            if ( t->fate >= T_FATE_MISSING && t->status == EXEC_CMD_OK &&
                !( t->flags & T_FLAG_INTERNAL ) )
            {
                TARGET * saved_includes;
                SETTINGS * s;
                saved_includes = t->includes;
                t->includes = 0;
                s = copysettings( t->settings );
                pushsettings( root_module(), s );
                headers( t );
                popsettings( root_module(), s );
                freesettings( s );
                if ( t->includes )
                {
                    make0( t->includes, t->parents->target, 0, 0, 0, t->includes
                        );
                    t->includes->includes = saved_includes;
                    for ( c = t->dependants.get(); c; c = c->next.get() )
                       targetentry( c->target->depends, t->includes );
                    additional_includes = t->includes;
                }
                else
                {
                    t->includes = saved_includes;
                }
            }
            if ( additional_includes )
                for ( c = t->parents.get(); c; c = c->next.get() )
                    push_state( &temp_stack, additional_includes, c->target,
                        T_STATE_MAKE1A );
            if ( t->scc_root )
            {
                TARGET * const scc_root = target_scc( t );
                assert( scc_root->progress < T_MAKE_DONE );
                for ( c = t->parents.get(); c; c = c->next.get() )
                {
                    if ( target_scc( c->target ) == scc_root )
                        push_state( &temp_stack, c->target, NULL, T_STATE_MAKE1B
                            );
                    else
                        targetentry( scc_root->parents, c->target );
                }
            }
            else
            {
                for ( c = t->parents.get(); c; c = c->next.get() )
                    push_state( &temp_stack, c->target, NULL, T_STATE_MAKE1B );
            }
            pop_state( &state_stack );
            push_stack_on_stack( &state_stack, &temp_stack );
        }
    }
}
static void call_timing_rule( TARGET * target, timing_info const * const time )
{
    LIST * timing_rule;
    pushsettings( root_module(), target->settings );
    timing_rule = var_get( root_module(), constant_TIMING_RULE );
    popsettings( root_module(), target->settings );
    if ( !list_empty( timing_rule ) )
    {
        FRAME frame[ 1 ];
        OBJECT * rulename = list_front( timing_rule );
        frame_init( frame );
        lol_add( frame->args, list_copy_range( timing_rule, list_next(
            list_begin( timing_rule ) ), list_end( timing_rule ) ) );
        lol_add( frame->args, list_new( object_copy( target->name ) ) );
        lol_add( frame->args, list_push_back( list_push_back( list_push_back( list_push_back( list_new(
            outf_time( &time->start ) ),
            outf_time( &time->end ) ),
            outf_double( time->user ) ),
            outf_double( time->system ) ),
            outf_double( timestamp_delta_seconds(&time->start, &time->end) ) )
            );
        list_free( evaluate_rule( bindrule( rulename , root_module() ), rulename, frame ) );
        frame_free( frame );
    }
}
static void call_action_rule
(
    TARGET * target,
    int32_t status,
    timing_info const * time,
    char const * executed_command,
    char const * command_output
)
{
    LIST * action_rule;
    pushsettings( root_module(), target->settings );
    action_rule = var_get( root_module(), constant_ACTION_RULE );
    popsettings( root_module(), target->settings );
    if ( !list_empty( action_rule ) )
    {
        FRAME frame[ 1 ];
        OBJECT * rulename = list_front( action_rule );
        frame_init( frame );
        lol_add( frame->args, list_copy_range( action_rule, list_next(
            list_begin( action_rule ) ), list_end( action_rule ) ) );
        lol_add( frame->args, list_new( object_copy( target->name ) ) );
        lol_add( frame->args,
            list_push_back( list_push_back( list_push_back( list_push_back( list_push_back( list_new(
                object_new( executed_command ) ),
                outf_int( status ) ),
                outf_time( &time->start ) ),
                outf_time( &time->end ) ),
                outf_double( time->user ) ),
                outf_double( time->system ) ) );
        if ( command_output )
        {
            OBJECT * command_output_obj = object_new( command_output );
            char * output_i = (char*)object_str(command_output_obj);
            for (; *output_i; ++output_i)
            {
                if (iscntrl(*output_i) && !isspace(*output_i)) *output_i = '?';
            }
            lol_add( frame->args, list_new( command_output_obj ) );
        }
        else
            lol_add( frame->args, L0 );
        list_free( evaluate_rule( bindrule( rulename, root_module() ), rulename, frame ) );
        frame_free( frame );
    }
}
static void make1c_closure
(
    void * const closure,
    int32_t status_orig,
    timing_info const * const time,
    char const * const cmd_stdout,
    char const * const cmd_stderr,
    int32_t const cmd_exit_reason
)
{
    TARGET * const t = (TARGET *)closure;
    CMD * const cmd = (CMD *)t->cmds;
    char const * rule_name = 0;
    char const * target_name = 0;
    assert( cmd );
    --cmdsrunning;
    {
        t->status = status_orig;
        if ( t->status == EXEC_CMD_FAIL && cmd->rule->actions->flags &
            RULE_IGNORE )
            t->status = EXEC_CMD_OK;
    }
    if ( DEBUG_MAKEQ ||
        ( DEBUG_MAKE && !( cmd->rule->actions->flags & RULE_QUIETLY ) ) )
    {
        rule_name = object_str( cmd->rule->name );
        target_name = object_str( list_front( lol_get( (LOL *)&cmd->args, 0 ) )
            );
    }
    if ( rule_name == NULL || globs.jobs > 1 )
        out_action( rule_name, target_name, cmd->buf->value, cmd_stdout,
            cmd_stderr, cmd_exit_reason );
    if ( cmd_exit_reason == EXIT_TIMEOUT && target_name )
        out_printf( "%ld second time limit exceeded\n", globs.timeout );
    out_flush();
    err_flush();
    if ( !globs.noexec )
    {
        call_timing_rule( t, time );
        if ( DEBUG_EXECCMD )
            out_printf( "%f sec system; %f sec user; %f sec clock\n",
                time->system, time->user,
                timestamp_delta_seconds(&time->start, &time->end) );
        call_action_rule( t, status_orig, time, cmd->buf->value, cmd_stdout );
    }
    if ( t->status == EXEC_CMD_FAIL && DEBUG_MAKE &&
        ! ( t->flags & T_FLAG_FAIL_EXPECTED ) )
    {
        if ( !DEBUG_EXEC )
            out_printf( "%s\n", cmd->buf->value );
        out_printf( "...failed %s ", object_str( cmd->rule->name ) );
        list_print( lol_get( (LOL *)&cmd->args, 0 ) );
        out_printf( "...\n" );
    }
    if ( t->status == EXEC_CMD_INTR )
    {
        ++intr;
        ++quit;
    }
    if ( t->status == EXEC_CMD_FAIL && globs.quitquick &&
        ! ( t->flags & T_FLAG_FAIL_EXPECTED ) )
        ++quit;
    if ( t->status != EXEC_CMD_OK )
    {
        LIST * const targets = lol_get( (LOL *)&cmd->args, 0 );
        LISTITER iter = list_begin( targets );
        LISTITER const end = list_end( targets );
        for ( ; iter != end; iter = list_next( iter ) )
        {
            char const * const filename = object_str( list_item( iter ) );
            TARGET const * const t = bindtarget( list_item( iter ) );
            if ( !( t->flags & T_FLAG_PRECIOUS ) && !unlink( filename ) )
                out_printf( "...removing %s\n", filename );
        }
    }
#ifdef OPT_SEMAPHORE
    cmd_sem_unlock( t );
#endif
    t->cmds = NULL;
    push_cmds( cmd->next, t->status );
    cmd_free( cmd );
}
static void push_cmds( CMDLIST * cmds, int32_t status )
{
    CMDLIST * cmd_iter;
    for( cmd_iter = cmds; cmd_iter; cmd_iter = cmd_iter->next )
    {
        if ( cmd_iter->iscmd )
        {
            CMD * next_cmd = cmd_iter->impl.cmd;
            if ( next_cmd->status < status )
                next_cmd->status = status;
            if ( --next_cmd->asynccnt == 0 )
            {
                TARGET * first_target = bindtarget( list_front( lol_get( &next_cmd->args, 0 ) ) );
                first_target->cmds = (char *)next_cmd;
                push_state( &state_stack, first_target, NULL, T_STATE_MAKE1C );
            }
            else if ( DEBUG_EXECCMD )
            {
                TARGET * first_target = bindtarget( list_front( lol_get( &next_cmd->args, 0 ) ) );
                out_printf( "Delaying %s %s: %d targets not ready\n", object_str( next_cmd->rule->name ), object_str( first_target->boundname ), next_cmd->asynccnt );
            }
        }
        else
        {
            TARGET * updated_target = cmd_iter->impl.t;
            if ( updated_target->status < status )
                updated_target->status = status;
            updated_target->cmds = NULL;
            push_state( &state_stack, updated_target, NULL, T_STATE_MAKE1C );
        }
    }
}
static void swap_settings
(
    module_t * * current_module,
    TARGET   * * current_target,
    module_t   * new_module,
    TARGET     * new_target
)
{
    if ( ( new_target == *current_target ) &&
        ( new_module == *current_module ) )
        return;
    if ( *current_target )
        popsettings( *current_module, (*current_target)->settings );
    if ( new_target )
        pushsettings( new_module, new_target->settings );
    *current_module = new_module;
    *current_target = new_target;
}
static CMD * make1cmds( TARGET * t )
{
    CMD * cmds = 0;
    CMD * last_cmd = 0;
    LIST * shell = L0;
    module_t * settings_module = 0;
    TARGET * settings_target = 0;
    ACTIONS * a0 = 0;
    int32_t const running_flag = globs.noexec ? A_RUNNING_NOEXEC : A_RUNNING;
    for ( a0 = t->actions; a0; a0 = a0->next )
    {
        RULE         * rule = a0->action->rule;
        rule_actions_ptr actions = rule->actions;
        SETTINGS     * boundvars;
        LIST         * nt;
        LIST         * ns;
        ACTIONS      * a1;
        if ( !actions )
            continue;
        if ( a0->action->running >= running_flag )
        {
            CMD * first;
            if ( a0->action->first_cmd == NULL )
                continue;
            first = (CMD *)a0->action->first_cmd;
            if( cmds )
            {
                last_cmd->next = cmdlist_append_cmd( last_cmd->next, first );
            }
            else
            {
                cmds = first;
            }
            last_cmd = (CMD *)a0->action->last_cmd;
            continue;
        }
        a0->action->running = running_flag;
        nt = make1list( L0, a0->action->targets, 0 );
        ns = make1list( L0, a0->action->sources, actions->flags );
        if ( actions->flags & RULE_TOGETHER )
            for ( a1 = a0->next; a1; a1 = a1->next )
                if ( a1->action->rule == rule &&
                    a1->action->running < running_flag &&
                    targets_equal( a0->action->targets, a1->action->targets ) )
                {
                    ns = make1list( ns, a1->action->sources, actions->flags );
                    a1->action->running = running_flag;
                }
        if ( list_empty( ns ) &&
            ( actions->flags & ( RULE_NEWSRCS | RULE_EXISTING ) ) )
        {
            list_free( nt );
            continue;
        }
        swap_settings( &settings_module, &settings_target, rule->module, t );
        if ( list_empty( shell ) )
        {
            shell = var_get( rule->module, constant_JAMSHELL );
        }
        boundvars = make1settings( rule->module, actions->bindlist );
        pushsettings( rule->module, boundvars );
        {
            int32_t const length = list_length( ns );
            int32_t start = 0;
            int32_t chunk = length;
            int32_t cmd_count = 0;
            targets_uptr semaphores;
            targets_ptr targets_iter;
            int32_t unique_targets;
            do
            {
                CMD * cmd;
                int32_t cmd_check_result;
                int32_t cmd_error_length;
                int32_t cmd_error_max_length;
                int32_t retry = 0;
                int32_t accept_command = 0;
                cmd = cmd_new( rule, list_copy( nt ), list_sublist( ns, start,
                    chunk ), list_copy( shell ) );
                cmd_check_result = exec_check( cmd->buf, &cmd->shell,
                    &cmd_error_length, &cmd_error_max_length );
                if ( cmd_check_result == EXEC_CHECK_OK )
                {
                    accept_command = 1;
                }
                else if ( cmd_check_result == EXEC_CHECK_NOOP )
                {
                    accept_command = 1;
                    cmd->noop = 1;
                }
                else if ( ( actions->flags & RULE_PIECEMEAL ) && ( chunk > 1 ) )
                {
                    assert( cmd_check_result == EXEC_CHECK_TOO_LONG ||
                        cmd_check_result == EXEC_CHECK_LINE_TOO_LONG );
                    chunk = chunk * 9 / 10;
                    retry = 1;
                }
                else
                {
                    char const * const error_message = cmd_check_result ==
                        EXEC_CHECK_TOO_LONG
                            ? "is too long"
                            : "contains a line that is too long";
                    assert( cmd_check_result == EXEC_CHECK_TOO_LONG ||
                        cmd_check_result == EXEC_CHECK_LINE_TOO_LONG );
                    out_printf(
                        "%s action %s (%d, max %d):\n",
                        object_str( rule->name ), error_message,
                        cmd_error_length, cmd_error_max_length );
                    out_puts( cmd->buf->value );
                    b2::clean_exit( EXITBAD );
                }
                assert( !retry || !accept_command );
                if ( accept_command )
                {
                    if ( cmds )
                    {
                        last_cmd->next = cmdlist_append_cmd( last_cmd->next, cmd );
                        last_cmd = cmd;
                    }
                    else
                    {
                        cmds = last_cmd = cmd;
                    }
                    if ( cmd_count++ == 0 )
                    {
                        a0->action->first_cmd = cmd;
                    }
                }
                else
                {
                    cmd_free( cmd );
                }
                if ( !retry )
                    start += chunk;
            }
            while ( start < length );
            a0->action->last_cmd = last_cmd;
            unique_targets = 0;
            for ( targets_iter = a0->action->targets.get(); targets_iter; targets_iter = targets_iter->next.get() )
            {
                if ( targets_contains( targets_iter->next, targets_iter->target ) )
                    continue;
                push_state( &state_stack, targets_iter->target, NULL, T_STATE_MAKE1A );
                ++unique_targets;
            }
            ( ( CMD * )a0->action->first_cmd )->asynccnt = unique_targets;
#if OPT_SEMAPHORE
            for ( targets_iter = a0->action->targets.get(); targets_iter; targets_iter = targets_iter->next.get() )
            {
                TARGET * sem = targets_iter->target->semaphore;
                if ( sem )
                {
                    if ( ! targets_contains( semaphores, sem ) )
                        targetentry( semaphores, sem );
                }
            }
            ( ( CMD * )a0->action->first_cmd )->lock = semaphores.get();
            ( ( CMD * )a0->action->last_cmd )->unlock = std::move(semaphores);
#endif
        }
        list_free( nt );
        list_free( ns );
        popsettings( rule->module, boundvars );
        freesettings( boundvars );
    }
    if ( cmds )
    {
        last_cmd->next = cmdlist_append_target( last_cmd->next, t );
    }
    swap_settings( &settings_module, &settings_target, 0, 0 );
    return cmds;
}
static LIST * make1list( LIST * l, const targets_uptr & ts, int32_t flags )
{
    targets_ptr targets = ts.get();
    for ( ; targets; targets = targets->next.get() )
    {
        TARGET * t = targets->target;
        if ( t->binding == T_BIND_UNBOUND )
            make1bind( t );
        if ( ( flags & RULE_EXISTING ) && ( flags & RULE_NEWSRCS ) )
        {
            if ( ( t->binding != T_BIND_EXISTS ) &&
                ( t->fate <= T_FATE_STABLE ) )
                continue;
        }
        else if ( flags & RULE_EXISTING )
        {
            if ( t->binding != T_BIND_EXISTS )
                continue;
        }
        else if ( flags & RULE_NEWSRCS )
        {
            if ( t->fate <= T_FATE_STABLE )
                continue;
        }
        if ( flags & RULE_TOGETHER )
        {
            LISTITER iter = list_begin( l );
            LISTITER const end = list_end( l );
            for ( ; iter != end; iter = list_next( iter ) )
                if ( object_equal( list_item( iter ), t->boundname ) )
                    break;
            if ( iter != end )
                continue;
        }
        l = list_push_back( l, object_copy( t->boundname ) );
    }
    return l;
}
static SETTINGS * make1settings( struct module_t * module, LIST * vars )
{
    SETTINGS * settings = 0;
    LISTITER vars_iter = list_begin( vars );
    LISTITER const vars_end = list_end( vars );
    for ( ; vars_iter != vars_end; vars_iter = list_next( vars_iter ) )
    {
        LIST * const l = var_get( module, list_item( vars_iter ) );
        LIST * nl = L0;
        LISTITER iter = list_begin( l );
        LISTITER const end = list_end( l );
        for ( ; iter != end; iter = list_next( iter ) )
        {
            TARGET * const t = bindtarget( list_item( iter ) );
            if ( t->binding == T_BIND_UNBOUND )
                make1bind( t );
            nl = list_push_back( nl, object_copy( t->boundname ) );
        }
        settings = addsettings( settings, VAR_SET, list_item( vars_iter ), nl );
    }
    return settings;
}
static void make1bind( TARGET * t )
{
    if ( t->flags & T_FLAG_NOTFILE )
        return;
    pushsettings( root_module(), t->settings );
    object_free( t->boundname );
    t->boundname = search( t->name, &t->time, 0, t->flags & T_FLAG_ISFILE );
    t->binding = timestamp_empty( &t->time ) ? T_BIND_MISSING : T_BIND_EXISTS;
    popsettings( root_module(), t->settings );
}
static bool targets_contains( const targets_uptr & ts, TARGET * t )
{
    targets_ptr l = ts.get();
    for ( ; l; l = l->next.get() )
    {
        if ( t == l->target )
        {
            return true;
        }
    }
    return false;
}
static bool targets_equal( const targets_uptr & ts1, const targets_uptr & ts2 )
{
    targets_ptr l1 = ts1.get();
    targets_ptr l2 = ts2.get();
    for ( ; l1 && l2; l1 = l1->next.get(), l2 = l2->next.get() )
    {
        if ( l1->target != l2->target )
            return false;
    }
    return !l1 && !l2;
}
#ifdef OPT_SEMAPHORE
static int32_t cmd_sem_lock( TARGET * t )
{
    CMD * cmd = (CMD *)t->cmds;
    targets_ptr iter;
    for ( iter = cmd->lock; iter; iter = iter->next.get() )
    {
        if ( iter->target->asynccnt > 0 )
        {
            if ( DEBUG_EXECCMD )
                out_printf( "SEM: %s is busy, delaying launch of %s\n",
                    object_str( iter->target->name ), object_str( t->name ) );
            targetentry( iter->target->parents, t );
            return 0;
        }
    }
    for ( iter = cmd->lock; iter; iter = iter->next.get() )
    {
        ++iter->target->asynccnt;
        if ( DEBUG_EXECCMD )
            out_printf( "SEM: %s now used by %s\n", object_str( iter->target->name
                ), object_str( t->name ) );
    }
    cmd->lock = NULL;
    return 1;
}
static void cmd_sem_unlock( TARGET * t )
{
    CMD * cmd = ( CMD * )t->cmds;
    targets_ptr iter;
    for ( iter = cmd->unlock.get(); iter; iter = iter->next.get() )
    {
        if ( DEBUG_EXECCMD )
            out_printf( "SEM: %s is now free\n", object_str(
                iter->target->name ) );
        --iter->target->asynccnt;
        assert( iter->target->asynccnt <= 0 );
    }
    for ( iter = cmd->unlock.get(); iter; iter = iter->next.get() )
    {
        while ( iter->target->parents )
        {
            TARGET * t1 = iter->target->parents->target;
            iter->target->parents = targets_pop(std::move(iter->target->parents));
            if ( cmd_sem_lock( t1 ) )
            {
                push_state( &state_stack, t1, NULL, T_STATE_MAKE1C );
                break;
            }
        }
    }
}
#endif
