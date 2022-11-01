#include "jam.h"
#include "patchlevel.h"
#define JAM_STRINGIZE(X) JAM_DO_STRINGIZE(X)
#define JAM_DO_STRINGIZE(X) #X
#define VERSION_MAJOR_SYM JAM_STRINGIZE(VERSION_MAJOR)
#define VERSION_MINOR_SYM JAM_STRINGIZE(VERSION_MINOR)
#define VERSION_PATCH_SYM JAM_STRINGIZE(VERSION_PATCH)
#define VERSION VERSION_MAJOR_SYM "." VERSION_MINOR_SYM
#define JAMVERSYM "JAMVERSION=" VERSION
#include "builtins.h"
#include "class.h"
#include "compile.h"
#include "constants.h"
#include "debugger.h"
#include "filesys.h"
#include "function.h"
#include "hcache.h"
#include "lists.h"
#include "make.h"
#include "object.h"
#include "option.h"
#include "output.h"
#include "parse.h"
#include "cwd.h"
#include "rules.h"
#include "scan.h"
#include "search.h"
#include "startup.h"
#include "jam_strings.h"
#include "timestamp.h"
#include "variable.h"
#include "execcmd.h"
#include "sysinfo.h"
#include <errno.h>
#include <string.h>
#ifdef OS_MAC
# include <QuickDraw.h>
#endif
#ifdef unix
# include <sys/utsname.h>
# include <signal.h>
#endif
struct globs globs =
{
    0,
    1,
    0,
    0,
    0,
#ifdef OS_MAC
    { 0, 0 },
#else
    { 0, 1 },
#endif
    0,
    0,
    0
};
static const char * othersyms[] = { OSMAJOR, OSMINOR, OSPLAT, JAMVERSYM, 0 };
#if defined( OS_NT ) && defined( __LCC__ )
# define use_environ _environ
#endif
#if defined( __MWERKS__)
# define use_environ _environ
    extern char * * _environ;
#endif
#ifndef use_environ
# define use_environ environ
# if !defined( __WATCOM__ ) && !defined( OS_OS2 ) && !defined( OS_NT )
    extern char **environ;
# endif
#endif
#if YYDEBUG != 0
    extern int yydebug;
#endif
#ifndef NDEBUG
static void run_unit_tests()
{
# if defined( USE_EXECNT )
    extern void execnt_unit_test();
    execnt_unit_test();
# endif
    string_unit_test();
}
#endif
int anyhow = 0;
void regex_done();
char const * saved_argv0;
static void usage( const char * progname )
{
	err_printf("\nusage: %s [ options ] targets...\n\n", progname);
	err_printf("-a      Build all targets, even if they are current.\n");
	err_printf("-dx     Set the debug level to x (0-13,console,mi).\n");
	err_printf("-fx     Read x instead of bootstrap.\n");
	err_printf("-jx     Run up to x shell commands concurrently.\n");
	err_printf("-lx     Limit actions to x number of seconds after which they are stopped.\n");
	err_printf("-mx     Maximum target output saved (kb), default is to save all output.\n");
	err_printf("-n      Don't actually execute the updating actions.\n");
	err_printf("-ox     Mirror all output to file x.\n");
	err_printf("-px     x=0, pipes action stdout and stderr merged into action output.\n");
	err_printf("-q      Quit quickly as soon as a target fails.\n");
	err_printf("-sx=y   Set variable x=y, overriding environment.\n");
	err_printf("-tx     Rebuild x, even if it is up-to-date.\n");
	err_printf("-v      Print the version of jam and exit.\n");
	err_printf("--x     Option is ignored.\n\n");
    b2::clean_exit( EXITBAD );
}
int guarded_main( int argc, char * * argv )
{
    int                     n;
    char                  * s;
    struct bjam_option      optv[ N_OPTS ];
    int                     status = 0;
    int                     arg_c = argc;
    char          *       * arg_v = argv;
    char            const * progname = argv[ 0 ];
    module_t              * environ_module;
    int                     is_debugger;
    b2::system_info sys_info;
    saved_argv0 = argv[ 0 ];
    last_update_now_status = 0;
#ifdef JAM_DEBUGGER
    is_debugger = 0;
    if ( getoptions( argc - 1, argv + 1, "-:l:m:d:j:p:f:gs:t:ano:qv", optv ) < 0 )
        usage( progname );
    if ( ( s = getoptval( optv, 'd', 0 ) ) )
    {
        if ( strcmp( s, "mi" ) == 0 )
        {
            debug_interface = DEBUG_INTERFACE_MI;
            is_debugger = 1;
        }
        else if ( strcmp( s, "console" ) == 0 )
        {
            debug_interface = DEBUG_INTERFACE_CONSOLE;
            is_debugger = 1;
        }
    }
#if NT
    if ( argc >= 3 )
    {
        size_t opt_len = strlen( debugger_opt );
        if ( strncmp( argv[ 1 ], debugger_opt, opt_len ) == 0 &&
            strncmp( argv[ 2 ], debugger_opt, opt_len ) == 0 )
        {
            debug_init_handles( argv[ 1 ] + opt_len, argv[ 2 ] + opt_len );
            arg_c = argc = (argc - 2);
            argv[ 2 ] = argv[ 0 ];
            arg_v = argv = (argv + 2);
            debug_interface = DEBUG_INTERFACE_CHILD;
        }
    }
    if ( is_debugger )
    {
        return debugger();
    }
#else
    if ( is_debugger )
    {
        if ( setjmp( debug_child_data.jmp ) != 0 )
        {
            arg_c = argc = debug_child_data.argc;
            arg_v = argv = (char * *)debug_child_data.argv;
            debug_interface = DEBUG_INTERFACE_CHILD;
        }
        else
        {
            return debugger();
        }
    }
#endif
#endif
    --argc;
    ++argv;
    #define OPTSTRING "-:l:m:d:j:p:f:gs:t:ano:qv"
    if ( getoptions( argc, argv, OPTSTRING, optv ) < 0 )
    {
        usage( progname );
    }
    globs.jobs = sys_info.cpu_thread_count();
    if ( ( s = getoptval( optv, 'v', 0 ) ) )
    {
        out_printf( "B2 Version %s. %s.\n", VERSION, OSMINOR );
        out_printf( "  Copyright 1993-2002 Christopher Seiwald and Perforce Software, Inc.\n" );
        out_printf( "  Copyright 2001 David Turner.\n" );
        out_printf( "  Copyright 2001-2004 David Abrahams.\n" );
        out_printf( "  Copyright 2002-2019 Rene Rivera.\n" );
        out_printf( "  Copyright 2003-2015 Vladimir Prus.\n" );
        out_printf( "\n  DEFAULTS: jobs = %i\n", globs.jobs);
        return EXITOK;
    }
    if ( ( s = getoptval( optv, 'n', 0 ) ) )
    {
        ++globs.noexec;
        globs.debug[ 2 ] = 1;
    }
    if ( ( s = getoptval( optv, 'p', 0 ) ) )
    {
        globs.pipe_action = atoi( s );
        if ( globs.pipe_action < 0 || 3 < globs.pipe_action )
        {
            err_printf( "Invalid pipe descriptor '%d', valid values are -p[0..3]."
                "\n", globs.pipe_action );
            b2::clean_exit( EXITBAD );
        }
    }
    if ( ( s = getoptval( optv, 'q', 0 ) ) )
        globs.quitquick = 1;
    if ( ( s = getoptval( optv, 'a', 0 ) ) )
        anyhow++;
    if ( ( s = getoptval( optv, 'j', 0 ) ) )
    {
        globs.jobs = atoi( s );
        if ( globs.jobs < 1 )
        {
            err_printf( "Invalid value for the '-j' option.\n" );
            b2::clean_exit( EXITBAD );
        }
    }
    if ( ( s = getoptval( optv, 'g', 0 ) ) )
        globs.newestfirst = 1;
    if ( ( s = getoptval( optv, 'l', 0 ) ) )
        globs.timeout = atoi( s );
    if ( ( s = getoptval( optv, 'm', 0 ) ) )
        globs.max_buf = atoi( s ) * 1024;
    for ( n = 0; ( s = getoptval( optv, 'd', n ) ); ++n )
    {
        int i;
        if ( !n )
            for ( i = 0; i < DEBUG_MAX; ++i )
                globs.debug[i] = 0;
        i = atoi( s );
        if ( ( i < 0 ) || ( i >= DEBUG_MAX ) )
        {
            out_printf( "Invalid debug level '%s'.\n", s );
            continue;
        }
        if ( *s == '+' )
            globs.debug[ i ] = 1;
        else while ( i )
            globs.debug[ i-- ] = 1;
    }
    if ( ( s = getoptval( optv, 'o', 0 ) ) )
    {
        if ( !( globs.out = fopen( s, "w" ) ) )
        {
            err_printf( "[errno %d] failed to write output file '%s': %s",
                errno, s, strerror(errno) );
            b2::clean_exit( EXITBAD );
        }
    }
    {
        PROFILE_ENTER( MAIN );
#ifndef NDEBUG
        run_unit_tests();
#endif
#if YYDEBUG != 0
        if ( DEBUG_PARSE )
            yydebug = 1;
#endif
        {
            timestamp current;
            timestamp_current( &current );
            var_set( root_module(), constant_JAMDATE, list_new( outf_time(
                &current ) ), VAR_SET );
        }
        var_set( root_module(), constant_JAM_VERSION,
                 list_push_back( list_push_back( list_new(
                   object_new( VERSION_MAJOR_SYM ) ),
                   object_new( VERSION_MINOR_SYM ) ),
                   object_new( VERSION_PATCH_SYM ) ),
                   VAR_SET );
#ifdef unix
        {
            struct utsname u;
            if ( uname( &u ) >= 0 )
            {
                var_set( root_module(), constant_JAMUNAME,
                         list_push_back(
                             list_push_back(
                                 list_push_back(
                                     list_push_back(
                                         list_new(
                                            object_new( u.sysname ) ),
                                         object_new( u.nodename ) ),
                                     object_new( u.release ) ),
                                 object_new( u.version ) ),
                             object_new( u.machine ) ), VAR_SET );
            }
        }
#endif
        {
            timestamp fmt_resolution[ 1 ];
            file_supported_fmt_resolution( fmt_resolution );
            var_set( root_module(), constant_JAM_TIMESTAMP_RESOLUTION, list_new(
                object_new( timestamp_timestr( fmt_resolution ) ) ), VAR_SET );
        }
        var_defines( root_module(), use_environ, 1 );
        environ_module = bindmodule( constant_ENVIRON );
        var_defines( environ_module, use_environ, 0 );
        var_defines( root_module(), othersyms, 1 );
        for ( n = 0; ( s = getoptval( optv, 's', n ) ); ++n )
        {
            char * symv[ 2 ];
            symv[ 0 ] = s;
            symv[ 1 ] = 0;
            var_defines( root_module(), symv, 1 );
            var_defines( environ_module, symv, 0 );
        }
        for ( n = 0; n < arg_c; ++n )
            var_set( root_module(), constant_ARGV, list_new( object_new(
                arg_v[ n ] ) ), VAR_APPEND );
        load_builtins();
        b2::startup::load_builtins();
        for ( n = 1; n < arg_c; ++n )
        {
            if ( arg_v[ n ][ 0 ] == '-' )
            {
                const char * f = "-:l:d:j:f:gs:t:ano:qv";
                for ( ; *f; ++f ) if ( *f == arg_v[ n ][ 1 ] ) break;
                if ( f[0] && f[1] && ( f[ 1 ] == ':' ) && ( arg_v[ n ][ 2 ] == '\0' ) ) ++n;
            }
            else
            {
                OBJECT * target = object_new( arg_v[ n ] );
                mark_target_for_updating( target );
                object_free( target );
            }
        }
        {
            LIST * const p = var_get( root_module(), constant_PARALLELISM );
            if ( !list_empty( p ) )
            {
                int const j = atoi( object_str( list_front( p ) ) );
                if ( j < 1 )
                    out_printf( "Invalid value of PARALLELISM: %s.\n",
                        object_str( list_front( p ) ) );
                else
                    globs.jobs = j;
            }
        }
        {
            LIST * const p = var_get( root_module(), constant_KEEP_GOING );
            if ( !list_empty( p ) )
                globs.quitquick = atoi( object_str( list_front( p ) ) ) ? 0 : 1;
        }
        if ( list_empty( targets_to_update() ) )
            mark_target_for_updating( constant_all );
        {
            FRAME frame[ 1 ];
            frame_init( frame );
            for ( n = 0; ( s = getoptval( optv, 'f', n ) ); ++n )
            {
                OBJECT * filename = object_new( s );
                parse_file( filename, frame );
                object_free( filename );
            }
            if ( !n  )
                status = b2::startup::bootstrap(frame) ? 0 : 13;
        }
        if ( status == 0 )
            status = yyanyerrors();
        if ( status == 0 )
        {
            for ( n = 0; ( s = getoptval( optv, 't', n ) ); ++n )
            {
                OBJECT * target = object_new( s );
                touch_target( target );
                object_free( target );
            }
            {
                PROFILE_ENTER( MAIN_MAKE );
                LIST * const targets = targets_to_update();
                if ( !list_empty( targets ) )
                    status |= make( targets, anyhow );
                else
                    status = last_update_now_status;
                PROFILE_EXIT( MAIN_MAKE );
            }
        }
        PROFILE_EXIT( MAIN );
    }
    return status ? EXITBAD : EXITOK;
}
int main( int argc, char * * argv )
{
    BJAM_MEM_INIT();
#ifdef OS_MAC
    InitGraf( &qd.thePort );
#endif
    cwd_init();
    constants_init();
    int result = EXIT_SUCCESS;
    try
    {
        result = guarded_main( argc, argv );
    }
    catch ( b2::exit_result exit_code )
    {
        result = (int)exit_code;
        out_flush();
        err_flush();
    }
    if ( DEBUG_PROFILE )
        profile_dump();
#ifdef OPT_HEADER_CACHE_EXT
    hcache_done();
#endif
    clear_targets_to_update();
    parse_done();
    debugger_done();
    property_set_done();
    exec_done();
    file_done();
    rules_done();
    timestamp_done();
    search_done();
    class_done();
    modules_done();
    regex_done();
    cwd_done();
    path_done();
    function_done();
    list_done();
    constants_done();
    object_done();
    if ( globs.out )
        fclose( globs.out );
    BJAM_MEM_CLOSE();
    return result;
}
