#include "jam.h"
#include "timestamp.h"
#include "filesys.h"
#include "hash.h"
#include "object.h"
#include "pathsys.h"
#include "jam_strings.h"
#include "output.h"
typedef struct _binding
{
    OBJECT * name;
    short flags;
#define BIND_SCANNED  0x01
    short progress;
#define BIND_INIT     0
#define BIND_NOENTRY  1
#define BIND_SPOTTED  2
#define BIND_MISSING  3
#define BIND_FOUND    4
    timestamp time;
} BINDING;
static struct hash * bindhash = 0;
#ifdef OS_NT
void timestamp_from_filetime( timestamp * const t, FILETIME const * const ft )
{
    static __int64 const secs_between_epochs = 11644473600;
    __int64 in;
    memcpy( &in, ft, sizeof( in ) );
    timestamp_init( t, (time_t)( ( in / 10000000 ) - secs_between_epochs ),
        (int)( in % 10000000 ) * 100 );
}
#endif
void timestamp_clear( timestamp * const time )
{
    time->secs = time->nsecs = 0;
}
int timestamp_cmp( timestamp const * const lhs, timestamp const * const rhs )
{
    return int(
        lhs->secs == rhs->secs
        ? lhs->nsecs - rhs->nsecs
        : lhs->secs - rhs->secs );
}
void timestamp_copy( timestamp * const target, timestamp const * const source )
{
    target->secs = source->secs;
    target->nsecs = source->nsecs;
}
void timestamp_current( timestamp * const t )
{
#ifdef OS_NT
    FILETIME ft;
    GetSystemTimeAsFileTime( &ft );
    timestamp_from_filetime( t, &ft );
#elif defined(_POSIX_TIMERS) && defined(CLOCK_REALTIME) && \
    (!defined(__GLIBC__) || (__GLIBC__ > 2) || (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 17))
    struct timespec ts;
    clock_gettime( CLOCK_REALTIME, &ts );
    timestamp_init( t, ts.tv_sec, ts.tv_nsec );
#else
    timestamp_init( t, time( 0 ), 0 );
#endif
}
int timestamp_empty( timestamp const * const time )
{
    return !time->secs && !time->nsecs;
}
void timestamp_from_path( timestamp * const time, OBJECT * const path )
{
    PROFILE_ENTER( timestamp );
    if ( file_time( path, time ) < 0 )
        timestamp_clear( time );
    PROFILE_EXIT( timestamp );
}
void timestamp_init( timestamp * const time, time_t const secs, int const nsecs
    )
{
    time->secs = secs;
    time->nsecs = nsecs;
}
void timestamp_max( timestamp * const max, timestamp const * const lhs,
    timestamp const * const rhs )
{
    if ( timestamp_cmp( lhs, rhs ) > 0 )
        timestamp_copy( max, lhs );
    else
        timestamp_copy( max, rhs );
}
static char const * timestamp_formatstr( timestamp const * const time,
    char const * const format )
{
    static char result1[ 500 ];
    static char result2[ 500 ];
    strftime( result1, sizeof( result1 ) / sizeof( *result1 ), format, gmtime(
        &time->secs ) );
    sprintf( result2, result1, time->nsecs );
    return result2;
}
char const * timestamp_str( timestamp const * const time )
{
    return timestamp_formatstr( time, "%Y-%m-%d %H:%M:%S.%%09d +0000" );
}
char const * timestamp_timestr( timestamp const * const time )
{
    return timestamp_formatstr( time, "%H:%M:%S.%%09d" );
}
static void free_timestamps( void * xbinding, void * data )
{
    object_free( ( (BINDING *)xbinding )->name );
}
void timestamp_done()
{
    if ( bindhash )
    {
        hashenumerate( bindhash, free_timestamps, 0 );
        hashdone( bindhash );
    }
}
double timestamp_delta_seconds( timestamp const * const a , timestamp const * const b )
{
    return difftime(b->secs, a->secs) + (b->nsecs - a->nsecs) * 1.0E-9;
}
