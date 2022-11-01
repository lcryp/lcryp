#ifndef BJAM_DEBUG_H
#define BJAM_DEBUG_H
#include "config.h"
#include "constants.h"
#include "object.h"
#include "psnip_debug_trap.h"
#include "output.h"
typedef struct profile_info
{
    OBJECT * name;
    double cumulative;
    double net;
    unsigned long num_entries;
    unsigned long stack_count;
    double memory;
} profile_info;
typedef struct profile_frame
{
    profile_info * info;
    double overhead;
    double entry_time;
    struct profile_frame * caller;
    double subrules;
} profile_frame;
profile_frame * profile_init( OBJECT * rulename, profile_frame * );
void profile_enter( OBJECT * rulename, profile_frame * );
void profile_memory( size_t mem );
void profile_exit( profile_frame * );
void profile_dump();
double profile_clock();
#define PROFILE_ENTER( scope ) profile_frame PROF_ ## scope, *PROF_ ## scope ## _p = profile_init( constant_ ## scope, &PROF_ ## scope )
#define PROFILE_EXIT( scope ) profile_exit( PROF_ ## scope ## _p )
OBJECT * profile_make_local( char const * );
#define PROFILE_ENTER_LOCAL( scope ) \
    static OBJECT * constant_LOCAL_##scope = 0; \
    if (DEBUG_PROFILE && !constant_LOCAL_##scope) constant_LOCAL_##scope = profile_make_local( #scope ); \
    PROFILE_ENTER( LOCAL_##scope )
#define PROFILE_EXIT_LOCAL( scope ) PROFILE_EXIT( LOCAL_##scope )
#define b2_cbreak(test) \
    if ( !static_cast<bool>( test ) ) \
    { \
        err_printf("%s: %d: %s: Assertion '%s' failed.\n", __FILE__, __LINE__, __FUNCTION__, #test); \
        err_flush(); \
        psnip_trap(); \
    } \
    while ( false )
#endif
