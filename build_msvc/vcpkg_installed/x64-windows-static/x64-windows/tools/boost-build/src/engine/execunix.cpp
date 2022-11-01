#include "jam.h"
#ifdef USE_EXECUNIX
#include "execcmd.h"
#include "lists.h"
#include "output.h"
#include "jam_strings.h"
#include "startup.h"
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <sys/resource.h>
#include <sys/times.h>
#include <sys/wait.h>
#include <poll.h>
#if defined(sun) || defined(__sun)
    #include <wait.h>
#endif
#include <sys/times.h>
#if defined(__APPLE__)
    #define NO_VFORK
#endif
#ifdef NO_VFORK
    #define vfork() fork()
#endif
static int get_free_cmdtab_slot();
static clock_t tps;
#define OUT 0
#define ERR 1
static struct cmdtab_t
{
    int          pid;
    int          fd[ 2 ];
    FILE      *  stream[ 2 ];
    clock_t      start_time;
    int          exit_reason;
    char      *  buffer[ 2 ];
    int          buf_size[ 2 ];
    timestamp    start_dt;
    int flags;
    ExecCmdCallback func;
    void * closure;
} * cmdtab = NULL;
static int cmdtab_size = 0;
struct pollfd * wait_fds = NULL;
#define WAIT_FDS_SIZE ( globs.jobs * ( globs.pipe_action ? 2 : 1 ) )
#define GET_WAIT_FD( job_idx ) ( wait_fds + ( ( job_idx * ( globs.pipe_action ? 2 : 1 ) ) ) )
void exec_init( void )
{
    int i;
    if ( globs.jobs > cmdtab_size )
    {
        cmdtab = (cmdtab_t*)BJAM_REALLOC( cmdtab, globs.jobs * sizeof( *cmdtab ) );
        memset( cmdtab + cmdtab_size, 0, ( globs.jobs - cmdtab_size ) * sizeof( *cmdtab ) );
        wait_fds = (pollfd*)BJAM_REALLOC( wait_fds, WAIT_FDS_SIZE * sizeof ( *wait_fds ) );
        for ( i = cmdtab_size; i < globs.jobs; ++i )
        {
            GET_WAIT_FD( i )[ OUT ].fd = -1;
            GET_WAIT_FD( i )[ OUT ].events = POLLIN;
            if ( globs.pipe_action )
            {
                GET_WAIT_FD( i )[ ERR ].fd = -1;
                GET_WAIT_FD( i )[ ERR ].events = POLLIN;
            }
        }
        cmdtab_size = globs.jobs;
    }
}
void exec_done( void )
{
    BJAM_FREE( cmdtab );
    BJAM_FREE( wait_fds );
}
int exec_check
(
    string const * command,
    LIST * * pShell,
    int32_t * error_length,
    int32_t * error_max_length
)
{
    int const is_raw_cmd = is_raw_command_request( *pShell );
    if ( !command->size && ( is_raw_cmd || list_empty( *pShell ) ) )
        return EXEC_CHECK_NOOP;
    return is_raw_cmd
        ? EXEC_CHECK_OK
        : check_cmd_for_too_long_lines( command->value, shell_maxline(), error_length,
            error_max_length );
}
#define EXECCMD_PIPE_READ 0
#define EXECCMD_PIPE_WRITE 1
void exec_cmd
(
    string const * command,
    int flags,
    ExecCmdCallback func,
    void * closure,
    LIST * shell
)
{
    struct sigaction ignore, saveintr, savequit;
    sigset_t chldmask, savemask;
    int const slot = get_free_cmdtab_slot();
    int out[ 2 ];
    int err[ 2 ];
    char const * argv[ MAXARGC + 1 ];
    static LIST * default_shell;
    if ( !default_shell )
        default_shell = list_push_back( list_new(
            object_new( "/bin/sh" ) ),
            object_new( "-c" ) );
    if ( list_empty( shell ) )
        shell = default_shell;
    argv_from_shell( argv, shell, command->value, slot );
    if ( DEBUG_EXECCMD )
    {
        int i;
        out_printf( "Using shell: " );
        list_print( shell );
        out_printf( "\n" );
        for ( i = 0; argv[ i ]; ++i )
            out_printf( "    argv[%d] = '%s'\n", i, argv[ i ] );
    }
    if ( pipe( out ) < 0 || ( globs.pipe_action && pipe( err ) < 0 ) )
    {
        errno_puts( "pipe" );
        b2::clean_exit( EXITBAD );
    }
    timestamp_current( &cmdtab[ slot ].start_dt );
    if ( 0 < globs.timeout )
    {
        struct tms buf;
        cmdtab[ slot ].start_time = times( &buf );
        if ( !tps ) tps = sysconf( _SC_CLK_TCK );
    }
    fcntl( out[ EXECCMD_PIPE_READ ], F_SETFD, FD_CLOEXEC );
    if ( globs.pipe_action )
        fcntl( err[ EXECCMD_PIPE_READ ], F_SETFD, FD_CLOEXEC );
    ignore.sa_handler = SIG_IGN;
    sigemptyset(&ignore.sa_mask);
    ignore.sa_flags = 0;
    if (sigaction(SIGINT, &ignore, &saveintr) < 0)
        return;
    if (sigaction(SIGQUIT, &ignore, &savequit) < 0)
        return;
    sigemptyset(&chldmask);
    sigaddset(&chldmask, SIGCHLD);
    if (sigprocmask(SIG_BLOCK, &chldmask, &savemask) < 0)
        return;
    if ( ( cmdtab[ slot ].pid = vfork() ) == -1 )
    {
        errno_puts( "vfork" );
        b2::clean_exit( EXITBAD );
    }
    if ( cmdtab[ slot ].pid == 0 )
    {
        int const pid = getpid();
        sigaction(SIGINT, &saveintr, NULL);
        sigaction(SIGQUIT, &savequit, NULL);
        sigprocmask(SIG_SETMASK, &savemask, NULL);
        dup2( out[ EXECCMD_PIPE_WRITE ], STDOUT_FILENO );
        dup2( globs.pipe_action ? err[ EXECCMD_PIPE_WRITE ] :
            out[ EXECCMD_PIPE_WRITE ], STDERR_FILENO );
        close( out[ EXECCMD_PIPE_WRITE ] );
        if ( globs.pipe_action )
            close( err[ EXECCMD_PIPE_WRITE ] );
        if ( 0 < globs.timeout )
        {
            struct rlimit r_limit;
            r_limit.rlim_cur = globs.timeout;
            r_limit.rlim_max = globs.timeout;
            setrlimit( RLIMIT_CPU, &r_limit );
        }
        if (0 != setpgid( pid, pid )) {
            errno_puts("setpgid(child)");
        }
        execvp( argv[ 0 ], (char * *)argv );
        errno_puts( "execvp" );
        _exit( 127 );
    }
    setpgid(cmdtab[ slot ].pid, cmdtab[ slot ].pid);
    close( out[ EXECCMD_PIPE_WRITE ] );
    if ( globs.pipe_action )
        close( err[ EXECCMD_PIPE_WRITE ] );
    fcntl( out[ EXECCMD_PIPE_READ ], F_SETFL, O_NONBLOCK );
    if ( globs.pipe_action )
        fcntl( err[ EXECCMD_PIPE_READ ], F_SETFL, O_NONBLOCK );
    cmdtab[ slot ].fd[ OUT ] = out[ EXECCMD_PIPE_READ ];
    cmdtab[ slot ].stream[ OUT ] = fdopen( cmdtab[ slot ].fd[ OUT ], "rb" );
    if ( !cmdtab[ slot ].stream[ OUT ] )
    {
        errno_puts( "fdopen" );
        b2::clean_exit( EXITBAD );
    }
    if ( globs.pipe_action )
    {
        cmdtab[ slot ].fd[ ERR ] = err[ EXECCMD_PIPE_READ ];
        cmdtab[ slot ].stream[ ERR ] = fdopen( cmdtab[ slot ].fd[ ERR ], "rb" );
        if ( !cmdtab[ slot ].stream[ ERR ] )
        {
            errno_puts( "fdopen" );
            b2::clean_exit( EXITBAD );
        }
    }
    GET_WAIT_FD( slot )[ OUT ].fd = out[ EXECCMD_PIPE_READ ];
    if ( globs.pipe_action )
        GET_WAIT_FD( slot )[ ERR ].fd = err[ EXECCMD_PIPE_READ ];
    cmdtab[ slot ].flags = flags;
    cmdtab[ slot ].func = func;
    cmdtab[ slot ].closure = closure;
    sigaction(SIGINT, &saveintr, NULL);
    sigaction(SIGQUIT, &savequit, NULL);
    sigprocmask(SIG_SETMASK, &savemask, NULL);
}
#undef EXECCMD_PIPE_READ
#undef EXECCMD_PIPE_WRITE
static int read_descriptor( int i, int s )
{
    int ret;
    char buffer[ BUFSIZ ];
    while ( 0 < ( ret = fread( buffer, sizeof( char ), BUFSIZ - 1,
        cmdtab[ i ].stream[ s ] ) ) )
    {
        buffer[ ret ] = 0;
        if ( ! ( cmdtab[ i ].flags & EXEC_CMD_QUIET ) )
        {
            if ( s == OUT && ( globs.pipe_action != 2 ) )
                out_data( buffer );
            else if ( s == ERR && ( globs.pipe_action & 2 ) )
                err_data( buffer );
        }
        if ( !cmdtab[ i ].buffer[ s ] )
        {
            if ( globs.max_buf && ret > globs.max_buf )
            {
                ret = globs.max_buf;
                buffer[ ret ] = 0;
            }
            cmdtab[ i ].buf_size[ s ] = ret + 1;
            cmdtab[ i ].buffer[ s ] = (char*)BJAM_MALLOC_ATOMIC( ret + 1 );
            memcpy( cmdtab[ i ].buffer[ s ], buffer, ret + 1 );
        }
        else
        {
            if ( cmdtab[ i ].buf_size[ s ] < globs.max_buf || !globs.max_buf )
            {
                char * tmp = cmdtab[ i ].buffer[ s ];
                int const old_len = cmdtab[ i ].buf_size[ s ] - 1;
                int const new_len = old_len + ret + 1;
                cmdtab[ i ].buf_size[ s ] = new_len;
                cmdtab[ i ].buffer[ s ] = (char*)BJAM_MALLOC_ATOMIC( new_len );
                memcpy( cmdtab[ i ].buffer[ s ], tmp, old_len );
                memcpy( cmdtab[ i ].buffer[ s ] + old_len, buffer, ret + 1 );
                BJAM_FREE( tmp );
            }
        }
    }
    if ( globs.max_buf && globs.max_buf <= cmdtab[ i ].buf_size[ s ] )
        cmdtab[ i ].buffer[ s ][ cmdtab[ i ].buf_size[ s ] - 2 ] = '\n';
    return feof( cmdtab[ i ].stream[ s ] );
}
static void close_streams( int const i, int const s )
{
    fclose( cmdtab[ i ].stream[ s ] );
    cmdtab[ i ].stream[ s ] = 0;
    close( cmdtab[ i ].fd[ s ] );
    cmdtab[ i ].fd[ s ] = 0;
    GET_WAIT_FD( i )[ s ].fd = -1;
}
void exec_wait()
{
    int finished = 0;
    while ( !finished )
    {
        int i;
        int select_timeout = globs.timeout;
        if ( globs.timeout > 0 )
        {
            struct tms buf;
            clock_t const current = times( &buf );
            for ( i = 0; i < globs.jobs; ++i )
                if ( cmdtab[ i ].pid )
                {
                    clock_t const consumed =
                        ( current - cmdtab[ i ].start_time ) / tps;
                    if ( consumed >= globs.timeout )
                    {
                        killpg( cmdtab[ i ].pid, SIGKILL );
                        cmdtab[ i ].exit_reason = EXIT_TIMEOUT;
                    }
                    else if ( globs.timeout - consumed < select_timeout )
                        select_timeout = globs.timeout - consumed;
                }
        }
        {
            int ret;
            int timeout;
            sigset_t sigmask;
            sigemptyset(&sigmask);
            sigaddset(&sigmask, SIGCHLD);
            sigprocmask(SIG_BLOCK, &sigmask, NULL);
            timeout = select_timeout? select_timeout * 1000 : -1;
            while ( ( ret = poll( wait_fds, WAIT_FDS_SIZE, timeout ) ) == -1 )
                if ( errno != EINTR )
                    break;
            sigprocmask(SIG_UNBLOCK, &sigmask, NULL);
            if ( ret <= 0 )
                continue;
        }
        for ( i = 0; i < globs.jobs; ++i )
        {
            int out_done = 0;
            int err_done = 0;
            if ( GET_WAIT_FD( i )[ OUT ].revents )
                out_done = read_descriptor( i, OUT );
            if ( globs.pipe_action && ( GET_WAIT_FD( i )[ ERR ].revents ) )
                err_done = read_descriptor( i, ERR );
            if ( out_done || err_done )
            {
                int pid;
                int status;
                int rstat;
                timing_info time_info;
                struct rusage cmd_usage;
                finished = 1;
                close_streams( i, OUT );
                if ( globs.pipe_action )
                    close_streams( i, ERR );
                while ( ( pid = wait4( cmdtab[ i ].pid, &status, 0, &cmd_usage ) ) == -1 )
                    if ( errno != EINTR )
                        break;
                if ( pid != cmdtab[ i ].pid )
                {
                    err_printf( "unknown pid %d with errno = %d\n", pid, errno );
                    b2::clean_exit( EXITBAD );
                }
                if ( WIFEXITED( status ) )
                    cmdtab[ i ].exit_reason = WEXITSTATUS( status )
                        ? EXIT_FAIL
                        : EXIT_OK;
                {
                    time_info.system = ((double)(cmd_usage.ru_stime.tv_sec)*1000000.0+(double)(cmd_usage.ru_stime.tv_usec))/1000000.0;
                    time_info.user   = ((double)(cmd_usage.ru_utime.tv_sec)*1000000.0+(double)(cmd_usage.ru_utime.tv_usec))/1000000.0;
                    timestamp_copy( &time_info.start, &cmdtab[ i ].start_dt );
                    timestamp_current( &time_info.end );
                }
                if ( interrupted() )
                    rstat = EXEC_CMD_INTR;
                else if ( status )
                    rstat = EXEC_CMD_FAIL;
                else
                    rstat = EXEC_CMD_OK;
                (*cmdtab[ i ].func)( cmdtab[ i ].closure, rstat, &time_info,
                    cmdtab[ i ].buffer[ OUT ], cmdtab[ i ].buffer[ ERR ],
                    cmdtab[ i ].exit_reason );
                BJAM_FREE( cmdtab[ i ].buffer[ OUT ] );
                cmdtab[ i ].buffer[ OUT ] = 0;
                cmdtab[ i ].buf_size[ OUT ] = 0;
                BJAM_FREE( cmdtab[ i ].buffer[ ERR ] );
                cmdtab[ i ].buffer[ ERR ] = 0;
                cmdtab[ i ].buf_size[ ERR ] = 0;
                cmdtab[ i ].pid = 0;
                cmdtab[ i ].func = 0;
                cmdtab[ i ].closure = 0;
                cmdtab[ i ].start_time = 0;
            }
        }
    }
}
static int get_free_cmdtab_slot()
{
    int slot;
    for ( slot = 0; slot < globs.jobs; ++slot )
        if ( !cmdtab[ slot ].pid )
            return slot;
    err_printf( "no slots for child!\n" );
    b2::clean_exit( EXITBAD );
    return -1;
}
int32_t shell_maxline()
{
    return MAXLINE;
}
# endif
