#include "jam.h"
#include "compile.h"
#include "builtins.h"
#include "class.h"
#include "constants.h"
#include "hash.h"
#include "hdrmacro.h"
#include "make.h"
#include "modules.h"
#include "parse.h"
#include "rules.h"
#include "search.h"
#include "jam_strings.h"
#include "variable.h"
#include "output.h"
#include "startup.h"
#include <assert.h>
#include <stdarg.h>
#include <string.h>
static void debug_compile( int which, char const * s, FRAME * );
void backtrace( FRAME * );
void backtrace_line( FRAME * );
void print_source_line( FRAME * );
void unknown_rule( FRAME *, char const * key, module_t *, OBJECT * rule_name );
LIST * evaluate_rule( RULE * rule, OBJECT * rulename, FRAME * frame )
{
    LIST          * result = L0;
    profile_frame   prof[ 1 ];
    module_t      * prev_module = frame->module;
    if ( DEBUG_COMPILE )
    {
        char buf[ 256 ] = "";
        if ( rule->module->name )
        {
            strncat( buf, object_str( rule->module->name ), sizeof( buf ) -
                1 );
            strncat( buf, ".", sizeof( buf ) - 1 );
            if ( strncmp( buf, object_str( rule->name ), strlen( buf ) ) == 0 )
            {
                buf[ 0 ] = 0;
            }
        }
        strncat( buf, object_str( rule->name ), sizeof( buf ) - 1 );
        debug_compile( 1, buf, frame );
        lol_print( frame->args );
        out_printf( "\n" );
    }
    if ( rule->procedure && rule->module != prev_module )
    {
        frame->module = rule->module;
    }
    if ( rule->procedure )
    {
        frame->rulename = object_str( rulename );
        if ( DEBUG_PROFILE )
            profile_enter( function_rulename( rule->procedure ), prof );
    }
    if ( !rule->actions && !rule->procedure )
        unknown_rule( frame, NULL, frame->module, rulename );
    if ( rule->actions )
    {
        targets_ptr t;
        ACTION * const action = b2::jam::make_ptr<ACTION>();
        action->rule = rule;
        action->targets.reset(); targetlist( action->targets, lol_get( frame->args, 0 ) );
        action->sources.reset(); targetlist( action->sources, lol_get( frame->args, 1 ) );
        action->refs = 1;
        if ( action->targets )
        {
            TARGET * const t0 = action->targets->target;
            for ( t = action->targets->next.get(); t; t = t->next.get() )
            {
                targetentry( t->target->rebuilds, t0 );
                targetentry( t0->rebuilds, t->target );
            }
        }
        for ( t = action->targets.get(); t; t = t->next.get() )
            t->target->actions = actionlist( t->target->actions, action );
        action_free( action );
    }
    if ( rule->procedure )
    {
        auto function = b2::jam::make_unique_bare_jptr( rule->procedure, function_refer, function_free );
        result = function_run( function.get(), frame );
    }
    if ( DEBUG_PROFILE && rule->procedure )
        profile_exit( prof );
    if ( DEBUG_COMPILE )
        debug_compile( -1, 0, frame );
    return result;
}
LIST * call_rule( OBJECT * rulename, FRAME * caller_frame, ... )
{
    va_list va;
    LIST * result;
    FRAME inner[ 1 ];
    frame_init( inner );
    inner->prev = caller_frame;
    inner->prev_user = caller_frame->module->user_module
        ? caller_frame
        : caller_frame->prev_user;
    inner->module = caller_frame->module;
    va_start( va, caller_frame );
    for ( ; ; )
    {
        LIST * const l = va_arg( va, LIST * );
        if ( !l )
            break;
        lol_add( inner->args, l );
    }
    va_end( va );
    result = evaluate_rule( bindrule( rulename, inner->module ), rulename, inner );
    frame_free( inner );
    return result;
}
static void debug_compile( int which, char const * s, FRAME * frame )
{
    static int level = 0;
    static char indent[ 36 ] = ">>>>|>>>>|>>>>|>>>>|>>>>|>>>>|>>>>|";
    if ( which >= 0 )
    {
        int i;
        print_source_line( frame );
        i = ( level + 1 ) * 2;
        while ( i > 35 )
        {
            out_puts( indent );
            i -= 35;
        }
        out_printf( "%*.*s ", i, i, indent );
    }
    if ( s )
        out_printf( "%s ", s );
    level += which;
}
