#include "jam.h"
#include "make.h"
#include "command.h"
#ifdef OPT_HEADER_CACHE_EXT
# include "hcache.h"
#endif
#include "headers.h"
#include "lists.h"
#include "object.h"
#include "parse.h"
#include "rules.h"
#include "search.h"
#include "timestamp.h"
#include "variable.h"
#include "execcmd.h"
#include "output.h"
#include <assert.h>
#ifndef max
# define max(a,b) ((a)>(b)?(a):(b))
#endif
static targets_uptr make0sort( targets_uptr c );
#ifdef OPT_GRAPH_DEBUG_EXT
    static void dependGraphOutput( TARGET * t, int32_t depth );
#endif
static char const * target_fate[] =
{
    "init",
    "making",
    "stable",
    "newer",
    "temp",
    "touched",
    "rebuild",
    "missing",
    "needtmp",
    "old",
    "update",
    "nofind",
    "nomake"
};
static char const * target_bind[] =
{
    "unbound",
    "missing",
    "parents",
    "exists",
};
#define spaces(x) ( ((const char *)"                    ") + ( x > 20 ? 0 : 20-x ) )
int32_t make( LIST * targets, int32_t anyhow )
{
    COUNTS counts[ 1 ];
    int32_t status = 0;
#ifdef OPT_HEADER_CACHE_EXT
    hcache_init();
#endif
    memset( (char *)counts, 0, sizeof( *counts ) );
    exec_init();
    bind_explicitly_located_targets();
    {
        LISTITER iter, end;
        PROFILE_ENTER( MAKE_MAKE0 );
        for ( iter = list_begin( targets ), end = list_end( targets ); iter != end; iter = list_next( iter ) )
        {
            TARGET * t = bindtarget( list_item( iter ) );
            if ( t->fate == T_FATE_INIT )
                make0( t, 0, 0, counts, anyhow, 0 );
        }
        PROFILE_EXIT( MAKE_MAKE0 );
    }
#ifdef OPT_GRAPH_DEBUG_EXT
    if ( DEBUG_GRAPH )
    {
        LISTITER iter, end;
        for ( iter = list_begin( targets ), end = list_end( targets ); iter != end; iter = list_next( iter ) )
           dependGraphOutput( bindtarget( list_item( iter ) ), 0 );
    }
#endif
    if ( DEBUG_MAKE )
    {
        if ( counts->targets )
            out_printf( "...found %d target%s...\n", counts->targets,
                counts->targets > 1 ? "s" : "" );
        if ( counts->temp )
            out_printf( "...using %d temp target%s...\n", counts->temp,
                counts->temp > 1 ? "s" : "" );
        if ( counts->updating )
            out_printf( "...updating %d target%s...\n", counts->updating,
                counts->updating > 1 ? "s" : "" );
        if ( counts->cantfind )
            out_printf( "...can't find %d target%s...\n", counts->cantfind,
                counts->cantfind > 1 ? "s" : "" );
        if ( counts->cantmake )
            out_printf( "...can't make %d target%s...\n", counts->cantmake,
                counts->cantmake > 1 ? "s" : "" );
    }
    status = counts->cantfind || counts->cantmake;
    {
        PROFILE_ENTER( MAKE_MAKE1 );
        status |= make1( targets );
        PROFILE_EXIT( MAKE_MAKE1 );
    }
    return status;
}
static void force_rebuilds( TARGET * t );
static void update_dependants( TARGET * t )
{
    targets_ptr q;
    for ( q = t->dependants.get(); q; q = q->next.get() )
    {
        TARGET * p = q->target;
        char fate0 = p->fate;
        if ( ( fate0 != T_FATE_INIT ) && ( fate0 < T_FATE_BUILD ) )
        {
            p->fate = T_FATE_UPDATE;
            if ( DEBUG_FATE )
            {
                out_printf( "fate change  %s from %s to %s (as dependent of %s)\n",
                        object_str( p->name ), target_fate[ (int32_t) fate0 ], target_fate[ (int32_t) p->fate ], object_str( t->name ) );
            }
            if ( fate0 > T_FATE_MAKING )
                update_dependants( p );
        }
    }
    force_rebuilds( t );
}
static void force_rebuilds( TARGET * t )
{
    targets_ptr d;
    for ( d = t->rebuilds.get(); d; d = d->next.get() )
    {
        TARGET * r = d->target;
        if ( r->fate < T_FATE_BUILD )
        {
            if ( DEBUG_FATE )
                out_printf( "fate change  %s from %s to %s (by rebuild)\n",
                        object_str( r->name ), target_fate[ (int32_t) r->fate ], target_fate[ T_FATE_REBUILD ] );
            r->fate = T_FATE_REBUILD;
            update_dependants( r );
        }
    }
}
int32_t make0rescan( TARGET * t, TARGET * rescanning )
{
    int32_t result = 0;
    targets_ptr c;
    if ( target_scc( t ) == rescanning )
        return 1;
    if ( t->rescanning == rescanning )
        return 0;
    if ( t->scc_root == NULL && t->progress > T_MAKE_ACTIVE )
        return 0;
    t->rescanning = rescanning;
    for ( c = t->depends.get(); c; c = c->next.get() )
    {
        TARGET * dependency = c->target;
        if ( target_scc( dependency ) != target_scc( t ) )
            dependency = target_scc( dependency );
        result |= make0rescan( dependency, rescanning );
        if ( c->target->includes == rescanning )
            result = 1;
    }
    if ( result && t->scc_root == NULL )
    {
        t->scc_root = rescanning;
        targetentry( rescanning->depends, t );
    }
    return result;
}
void make0
(
    TARGET * t,
    TARGET * p,
    int32_t      depth,
    COUNTS * counts,
    int32_t      anyhow,
    TARGET * rescanning
)
{
    targets_ptr c;
    TARGET     * ptime = t;
    TARGET     * located_target = 0;
    timestamp    last;
    timestamp    leaf;
    timestamp    hlast;
    int32_t          fate;
    char const * flag = "";
    SETTINGS   * s;
#ifdef OPT_GRAPH_DEBUG_EXT
    int32_t savedFate;
    int32_t oldTimeStamp;
#endif
    if ( DEBUG_MAKEPROG )
        out_printf( "make\t--\t%s%s\n", spaces( depth ), object_str( t->name ) );
    if ( DEBUG_MAKEPROG )
        out_printf( "make\t--\t%s%s\n", spaces( depth ), object_str( t->name ) );
    t->fate = T_FATE_MAKING;
    t->depth = depth;
    s = copysettings( t->settings );
    pushsettings( root_module(), s );
    if ( ( t->binding == T_BIND_UNBOUND ) && !( t->flags & T_FLAG_NOTFILE ) )
    {
        OBJECT * another_target;
        object_free( t->boundname );
        t->boundname = search( t->name, &t->time, &another_target,
            t->flags & T_FLAG_ISFILE );
        if ( another_target )
            located_target = bindtarget( another_target );
        t->binding = timestamp_empty( &t->time )
            ? T_BIND_MISSING
            : T_BIND_EXISTS;
    }
    if ( p && ( t->flags & T_FLAG_INTERNAL ) )
        ptime = p;
    if ( p && ( t->flags & T_FLAG_TEMP ) &&
        ( t->binding == T_BIND_MISSING ) &&
        ( p->binding != T_BIND_MISSING ) )
    {
        t->binding = T_BIND_PARENTS;
        ptime = p;
    }
#ifdef OPT_SEMAPHORE
    {
        LIST * var = var_get( root_module(), constant_JAM_SEMAPHORE );
        if ( !list_empty( var ) )
        {
            TARGET * const semaphore = bindtarget( list_front( var ) );
            semaphore->progress = T_MAKE_SEMAPHORE;
            t->semaphore = semaphore;
        }
    }
#endif
    if ( t->binding == T_BIND_EXISTS )
        headers( t );
    popsettings( root_module(), s );
    freesettings( s );
    if ( DEBUG_BIND )
    {
        if ( !object_equal( t->name, t->boundname ) )
            out_printf( "bind\t--\t%s%s: %s\n", spaces( depth ),
                object_str( t->name ), object_str( t->boundname ) );
        switch ( t->binding )
        {
        case T_BIND_UNBOUND:
        case T_BIND_MISSING:
        case T_BIND_PARENTS:
            out_printf( "time\t--\t%s%s: %s\n", spaces( depth ),
                object_str( t->name ), target_bind[ (int32_t)t->binding ] );
            break;
        case T_BIND_EXISTS:
            out_printf( "time\t--\t%s%s: %s\n", spaces( depth ),
                object_str( t->name ), timestamp_str( &t->time ) );
            break;
        }
    }
    for ( c = t->depends.get(); c; c = c->next.get() )
    {
        int32_t const internal = t->flags & T_FLAG_INTERNAL;
        if ( c->target->fate == T_FATE_INIT )
            make0( c->target, ptime, depth + 1, counts, anyhow, rescanning );
        else if ( c->target->fate == T_FATE_MAKING && !internal )
            out_printf( "warning: %s depends on itself\n", object_str(
                c->target->name ) );
        else if ( c->target->fate != T_FATE_MAKING && rescanning )
            make0rescan( c->target, rescanning );
        if ( rescanning && c->target->includes && c->target->includes->fate !=
            T_FATE_MAKING )
            make0rescan( target_scc( c->target->includes ), rescanning );
    }
    if ( located_target )
    {
        if ( located_target->fate == T_FATE_INIT )
            make0( located_target, ptime, depth + 1, counts, anyhow, rescanning
                );
        else if ( located_target->fate != T_FATE_MAKING && rescanning )
            make0rescan( located_target, rescanning );
    }
    if ( t->includes )
        make0( t->includes, p, depth + 1, counts, anyhow, rescanning );
    {
        targets_uptr incs;
        for ( c = t->depends.get(); c; c = c->next.get() )
            if ( c->target->includes )
                targetentry( incs, c->target->includes );
        t->depends = targetchain( std::move(t->depends), std::move(incs) );
    }
    if ( located_target )
        targetentry( t->depends, located_target );
    {
        int32_t cycle_depth = depth;
        for ( c = t->depends.get(); c; c = c->next.get() )
        {
            TARGET * scc_root = target_scc( c->target );
            if ( scc_root->fate == T_FATE_MAKING &&
                ( !scc_root->includes ||
                  scc_root->includes->fate != T_FATE_MAKING ) )
            {
                if ( scc_root->depth < cycle_depth )
                {
                    cycle_depth = scc_root->depth;
                    t->scc_root = scc_root;
                }
            }
        }
    }
    timestamp_clear( &last );
    timestamp_clear( &leaf );
    fate = T_FATE_STABLE;
    for ( c = t->depends.get(); c; c = c->next.get() )
    {
        if ( c->target->scc_root )
        {
            TARGET * const scc_root = target_scc( c->target );
            if ( scc_root != t->scc_root )
            {
                timestamp_max( &c->target->leaf, &c->target->leaf,
                    &scc_root->leaf );
                timestamp_max( &c->target->time, &c->target->time,
                    &scc_root->time );
                c->target->fate = max( c->target->fate, scc_root->fate );
            }
        }
        timestamp_max( &leaf, &leaf, &c->target->leaf );
        if ( t->flags & T_FLAG_LEAVES )
        {
            timestamp_copy( &last, &leaf );
            continue;
        }
        timestamp_max( &last, &last, &c->target->time );
        fate = max( fate, c->target->fate );
#ifdef OPT_GRAPH_DEBUG_EXT
        if ( DEBUG_FATE )
            if ( fate < c->target->fate )
                out_printf( "fate change %s from %s to %s by dependency %s\n",
                    object_str( t->name ), target_fate[ (int32_t)fate ],
                    target_fate[ (int32_t)c->target->fate ], object_str(
                    c->target->name ) );
#endif
    }
    if ( t->includes )
        timestamp_copy( &hlast, &t->includes->time );
    else
        timestamp_clear( &hlast );
    if ( t->flags & T_FLAG_NOUPDATE )
    {
#ifdef OPT_GRAPH_DEBUG_EXT
        if ( DEBUG_FATE )
            if ( fate != T_FATE_STABLE )
                out_printf( "fate change  %s back to stable, NOUPDATE.\n",
                    object_str( t->name ) );
#endif
        timestamp_clear( &last );
        timestamp_clear( &t->time );
        fate = T_FATE_STABLE;
    }
#ifdef OPT_GRAPH_DEBUG_EXT
    savedFate = fate;
    oldTimeStamp = 0;
#endif
    if ( fate >= T_FATE_BROKEN )
    {
        fate = T_FATE_CANTMAKE;
    }
    else if ( fate >= T_FATE_SPOIL )
    {
        fate = T_FATE_UPDATE;
    }
    else if ( t->binding == T_BIND_MISSING )
    {
        fate = T_FATE_MISSING;
    }
    else if ( t->binding == T_BIND_EXISTS && timestamp_cmp( &last, &t->time ) >
        0 )
    {
#ifdef OPT_GRAPH_DEBUG_EXT
        oldTimeStamp = 1;
#endif
        fate = T_FATE_OUTDATED;
    }
    else if ( t->binding == T_BIND_PARENTS && timestamp_cmp( &last, &p->time ) >
        0 )
    {
#ifdef OPT_GRAPH_DEBUG_EXT
        oldTimeStamp = 1;
#endif
        fate = T_FATE_NEEDTMP;
    }
    else if ( t->binding == T_BIND_PARENTS && timestamp_cmp( &hlast, &p->time )
        > 0 )
    {
        fate = T_FATE_NEEDTMP;
    }
    else if ( t->flags & T_FLAG_TOUCHED )
    {
        fate = T_FATE_TOUCHED;
    }
    else if ( anyhow && !( t->flags & T_FLAG_NOUPDATE ) )
    {
        fate = T_FATE_TOUCHED;
    }
    else if ( t->binding == T_BIND_EXISTS && ( t->flags & T_FLAG_TEMP ) )
    {
        fate = T_FATE_ISTMP;
    }
    else if ( t->binding == T_BIND_EXISTS && p && p->binding != T_BIND_UNBOUND
        && timestamp_cmp( &t->time, &p->time ) > 0 )
    {
#ifdef OPT_GRAPH_DEBUG_EXT
        oldTimeStamp = 1;
#endif
        fate = T_FATE_NEWER;
    }
    else
    {
        fate = T_FATE_STABLE;
    }
#ifdef OPT_GRAPH_DEBUG_EXT
    if ( DEBUG_FATE && ( fate != savedFate ) )
	{
        if ( savedFate == T_FATE_STABLE )
            out_printf( "fate change  %s set to %s%s\n", object_str( t->name ),
                target_fate[ fate ], oldTimeStamp ? " (by timestamp)" : "" );
        else
            out_printf( "fate change  %s from %s to %s%s\n", object_str( t->name ),
                target_fate[ savedFate ], target_fate[ fate ], oldTimeStamp ?
                " (by timestamp)" : "" );
	}
#endif
    if ( ( fate == T_FATE_MISSING ) && !t->actions && !t->depends )
    {
        if ( t->flags & T_FLAG_NOCARE )
        {
#ifdef OPT_GRAPH_DEBUG_EXT
            if ( DEBUG_FATE )
                out_printf( "fate change %s to STABLE from %s, "
                    "no actions, no dependencies and do not care\n",
                    object_str( t->name ), target_fate[ fate ] );
#endif
            fate = T_FATE_STABLE;
        }
        else
        {
            out_printf( "don't know how to make %s\n", object_str( t->name ) );
            fate = T_FATE_CANTFIND;
        }
    }
    timestamp_max( &t->time, &t->time, &last );
    timestamp_copy( &t->leaf, timestamp_empty( &leaf ) ? &t->time : &leaf );
    if ( fate > t->fate )
        t->fate = fate;
    else
        fate = t->fate;
    if ( ( fate >= T_FATE_BUILD ) && ( fate < T_FATE_BROKEN ) )
    {
        ACTIONS * a;
        targets_ptr c;
        for ( a = t->actions; a; a = a->next )
        {
            for ( c = a->action->targets.get(); c; c = c->next.get() )
            {
                if ( c->target->fate == T_FATE_INIT )
                {
                    make0( c->target, ptime, depth + 1, counts, anyhow, rescanning );
                }
            }
        }
    }
    if ( ( fate >= T_FATE_BUILD ) && ( fate < T_FATE_BROKEN ) )
        force_rebuilds( t );
    if ( globs.newestfirst )
        t->depends = make0sort( std::move(t->depends) );
    if ( t->flags & T_FLAG_INTERNAL )
        return;
    if ( counts )
    {
#ifdef OPT_IMPROVED_PATIENCE_EXT
        ++counts->targets;
#else
        if ( !( ++counts->targets % 1000 ) && DEBUG_MAKE )
        {
            out_printf( "...patience...\n" );
            out_flush();
        }
#endif
        if ( fate == T_FATE_ISTMP )
            ++counts->temp;
        else if ( fate == T_FATE_CANTFIND )
            ++counts->cantfind;
        else if ( ( fate == T_FATE_CANTMAKE ) && t->actions )
            ++counts->cantmake;
        else if ( ( fate >= T_FATE_BUILD ) && ( fate < T_FATE_BROKEN ) &&
            t->actions )
            ++counts->updating;
    }
    if ( !( t->flags & T_FLAG_NOTFILE ) && ( fate >= T_FATE_SPOIL ) )
        flag = "+";
    else if ( t->binding == T_BIND_EXISTS && p && timestamp_cmp( &t->time,
        &p->time ) > 0 )
        flag = "*";
    if ( DEBUG_MAKEPROG )
        out_printf( "made%s\t%s\t%s%s\n", flag, target_fate[ (int32_t)t->fate ],
            spaces( depth ), object_str( t->name ) );
}
#ifdef OPT_GRAPH_DEBUG_EXT
static char const * target_name( TARGET * t )
{
    static char buf[ 1000 ];
    if ( t->flags & T_FLAG_INTERNAL )
    {
        sprintf( buf, "%s (internal node)", object_str( t->name ) );
        return buf;
    }
    return object_str( t->name );
}
static void dependGraphOutput( TARGET * t, int32_t depth )
{
    targets_ptr c;
    if ( ( t->flags & T_FLAG_VISITED ) || !t->name || !t->boundname )
        return;
    t->flags |= T_FLAG_VISITED;
    switch ( t->fate )
    {
        case T_FATE_TOUCHED:
        case T_FATE_MISSING:
        case T_FATE_OUTDATED:
        case T_FATE_UPDATE:
            out_printf( "->%s%2d Name: %s\n", spaces( depth ), depth, target_name( t
                ) );
            break;
        default:
            out_printf( "  %s%2d Name: %s\n", spaces( depth ), depth, target_name( t
                ) );
            break;
    }
    if ( !object_equal( t->name, t->boundname ) )
        out_printf( "  %s    Loc: %s\n", spaces( depth ), object_str( t->boundname )
            );
    switch ( t->fate )
    {
    case T_FATE_STABLE:
        out_printf( "  %s       : Stable\n", spaces( depth ) );
        break;
    case T_FATE_NEWER:
        out_printf( "  %s       : Newer\n", spaces( depth ) );
        break;
    case T_FATE_ISTMP:
        out_printf( "  %s       : Up to date temp file\n", spaces( depth ) );
        break;
    case T_FATE_NEEDTMP:
        out_printf( "  %s       : Temporary file, to be updated\n", spaces( depth )
            );
        break;
    case T_FATE_TOUCHED:
        out_printf( "  %s       : Been touched, updating it\n", spaces( depth ) );
        break;
    case T_FATE_MISSING:
        out_printf( "  %s       : Missing, creating it\n", spaces( depth ) );
        break;
    case T_FATE_OUTDATED:
        out_printf( "  %s       : Outdated, updating it\n", spaces( depth ) );
        break;
    case T_FATE_REBUILD:
        out_printf( "  %s       : Rebuild, updating it\n", spaces( depth ) );
        break;
    case T_FATE_UPDATE:
        out_printf( "  %s       : Updating it\n", spaces( depth ) );
        break;
    case T_FATE_CANTFIND:
        out_printf( "  %s       : Can not find it\n", spaces( depth ) );
        break;
    case T_FATE_CANTMAKE:
        out_printf( "  %s       : Can make it\n", spaces( depth ) );
        break;
    }
    if ( t->flags & ~T_FLAG_VISITED )
    {
        out_printf( "  %s       : ", spaces( depth ) );
        if ( t->flags & T_FLAG_TEMP     ) out_printf( "TEMPORARY " );
        if ( t->flags & T_FLAG_NOCARE   ) out_printf( "NOCARE "    );
        if ( t->flags & T_FLAG_NOTFILE  ) out_printf( "NOTFILE "   );
        if ( t->flags & T_FLAG_TOUCHED  ) out_printf( "TOUCHED "   );
        if ( t->flags & T_FLAG_LEAVES   ) out_printf( "LEAVES "    );
        if ( t->flags & T_FLAG_NOUPDATE ) out_printf( "NOUPDATE "  );
        out_printf( "\n" );
    }
    for ( c = t->depends.get(); c; c = c->next.get() )
    {
        out_printf( "  %s       : Depends on %s (%s)", spaces( depth ),
           target_name( c->target ), target_fate[ (int32_t)c->target->fate ] );
        if ( !timestamp_cmp( &c->target->time, &t->time ) )
            out_printf( " (max time)");
        out_printf( "\n" );
    }
    for ( c = t->depends.get(); c; c = c->next.get() )
        dependGraphOutput( c->target, depth + 1 );
}
#endif
static targets_uptr make0sort( targets_uptr chain )
{
    PROFILE_ENTER( MAKE_MAKE0SORT );
    for ( targets_ptr front = chain.get(); front ; front = front->next.get() )
    {
        targets_ptr newest = front->next.get();
        for ( targets_ptr rest = newest; rest ; rest = rest->next.get())
        {
            if ( timestamp_cmp( &newest->target->time, &rest->target->time ) > 0 )
                newest = rest;
        }
        if ( front != newest )
            std::swap( front->target, newest->target );
    }
    PROFILE_EXIT( MAKE_MAKE0SORT );
    return chain;
}
static LIST * targets_to_update_ = L0;
void mark_target_for_updating( OBJECT * target )
{
    targets_to_update_ = list_push_back( targets_to_update_, object_copy(
        target ) );
}
LIST * targets_to_update()
{
    return targets_to_update_;
}
void clear_targets_to_update()
{
    list_free( targets_to_update_ );
    targets_to_update_ = L0;
}
