#include "jam.h"
#include "lists.h"
#include "execcmd.h"
#include "output.h"
#include "startup.h"
#ifdef OS_VMS
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <times.h>
#include <unistd.h>
#include <errno.h>
#define WRTLEN 240
#define MIN( a, b ) ((a) < (b) ? (a) : (b))
#define CHAR_DQUOTE  '"'
#define VMS_PATH_MAX 1024
#define VMS_COMMAND_MAX  1024
#define VMS_WARNING 0
#define VMS_SUCCESS 1
#define VMS_ERROR   2
#define VMS_FATAL   4
char commandbuf[ VMS_COMMAND_MAX ] = { 0 };
static int get_status(int vms_status);
static clock_t get_cpu_time();
int exec_check
(
    string const * command,
    LIST * * pShell,
    int32_t * error_length,
    int32_t * error_max_length
)
{
    int const is_raw_cmd = 1;
    if ( !command->size && ( is_raw_cmd || list_empty( *pShell ) ) )
        return EXEC_CHECK_NOOP;
    return is_raw_cmd
        ? EXEC_CHECK_OK
        : check_cmd_for_too_long_lines( command->value, shell_maxline(), error_length,
            error_max_length );
}
void exec_cmd
(
    string const * command,
    int flags,
    ExecCmdCallback func,
    void * closure,
    LIST * shell
)
{
    char * s;
    char * e;
    char * p;
    int vms_status;
    int status;
    int rstat = EXEC_CMD_OK;
    int exit_reason = EXIT_OK;
    timing_info time_info;
    timestamp start_dt;
    struct tms start_time;
    struct tms end_time;
    char * cmd_string = command->value;
    timestamp_current( &time_info.start );
    times( &start_time );
    for ( s = cmd_string; *s && isspace( *s ); ++s );
    e = p = strchr( s, '\n' );
    while ( p && isspace( *p ) )
        ++p;
    if ( ( p && *p ) || ( e - s > WRTLEN ) )
    {
        FILE * f;
        if ( !*commandbuf )
        {
            OBJECT * tmp_filename = 0;
            tmp_filename = path_tmpfile();
            {
                string os_filename[ 1 ];
                string_new( os_filename );
                path_translate_to_os( object_str( tmp_filename ), os_filename );
                object_free( tmp_filename );
                tmp_filename = object_new( os_filename->value );
                string_free( os_filename );
            }
            commandbuf[0] = '@';
            strncat( commandbuf + 1, object_str( tmp_filename ),
                     VMS_COMMAND_MAX - 2);
        }
        if ( !( f = fopen( commandbuf + 1, "w" ) ) )
        {
            err_printf( "[errno %d] failed to wite cmd_string file '%s': %s",
                errno, commandbuf + 1, strerror(errno) );
            rstat = EXEC_CMD_FAIL;
            exit_reason = EXIT_FAIL;
            times( &end_time );
            timestamp_current( &time_info.end );
            time_info.system = (double)( end_time.tms_cstime -
                start_time.tms_cstime ) / 100.;
            time_info.user   = (double)( end_time.tms_cutime -
                start_time.tms_cutime ) / 100.;
            (*func)( closure, rstat, &time_info, "" , "", exit_reason  );
            return;
        }
        {
            char * cwd = NULL;
            int cwd_buf_size = VMS_PATH_MAX;
            while ( !(cwd = getcwd( NULL, cwd_buf_size ) )
                     && errno == ERANGE )
            {
                cwd_buf_size += VMS_PATH_MAX;
            }
            if ( !cwd )
            {
                errno_puts( "can not get current working directory" );
                b2::clean_exit( EXITBAD );
            }
            fprintf( f, "$ SET DEFAULT %s\n", cwd);
            free( cwd );
        }
        while ( *cmd_string )
        {
            char * s = strchr( cmd_string,'\n' );
            int len = s ? s + 1 - cmd_string : strlen( cmd_string );
            fputc( '$', f );
            while ( len > 0 )
            {
                char * q = cmd_string;
                char * qe = cmd_string + MIN( len, WRTLEN );
                char * qq = q;
                int quote = 0;
                for ( ; q < qe; ++q )
                    if ( ( *q == CHAR_DQUOTE ) && ( quote = !quote ) )
                        qq = q;
                if (  len > WRTLEN && quote )
                {
                    q = qq;
                    if ( q == cmd_string )
                    {
                        for ( q = qe; q < ( cmd_string + len )
                                      && *q != CHAR_DQUOTE ; ++q) {}
                        q = ( *q == CHAR_DQUOTE) ? ( q + 1 ) : ( cmd_string + len );
                    }
                }
                fwrite( cmd_string, ( q - cmd_string ), 1, f );
                len -= ( q - cmd_string );
                cmd_string = q;
                if ( len )
                {
                    fputc( '-', f );
                    fputc( '\n', f );
                }
            }
        }
        fclose( f );
        if ( DEBUG_EXECCMD )
        {
            FILE * f;
            char buf[ WRTLEN + 1 ] = { 0 };
            if ( (f = fopen( commandbuf + 1, "r" ) ) )
            {
                int nbytes;
                printf( "Command file: %s\n", commandbuf + 1 );
                do
                {
                    nbytes = fread( buf, sizeof( buf[0] ), sizeof( buf ) - 1, f );
                    if ( nbytes ) fwrite(buf, sizeof( buf[0] ), nbytes, stdout);
                }
                while ( !feof(f) );
                fclose(f);
            }
        }
        vms_status = system( commandbuf );
        status = get_status( vms_status );
        unlink( commandbuf + 1 );
    }
    else
    {
        if ( e ) *e = 0;
        status = VMS_SUCCESS;
        if ( *s )
        {
            vms_status = system( s );
            status = get_status( vms_status );
        }
    }
    times( &end_time );
    timestamp_current( &time_info.end );
    time_info.system = (double)( end_time.tms_cstime -
        start_time.tms_cstime ) / 100.;
    time_info.user   = (double)( end_time.tms_cutime -
        start_time.tms_cutime ) / 100.;
    if ( ( status == VMS_ERROR ) || ( status == VMS_FATAL ) )
    {
        rstat = EXEC_CMD_FAIL;
        exit_reason = EXIT_FAIL;
    }
    (*func)( closure, rstat, &time_info, "" , "", exit_reason  );
}
void exec_wait()
{
    return;
}
int get_status( int vms_status )
{
#define VMS_STATUS_DCL_IVVERB 0x00038090
    int status;
    switch (vms_status)
    {
    case VMS_STATUS_DCL_IVVERB:
        status = VMS_ERROR;
        break;
    default:
        status = vms_status & 0x07;
    }
    return status;
}
#define __NEW_STARLET 1
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <ssdef.h>
#include <stsdef.h>
#include <jpidef.h>
#include <efndef.h>
#include <iosbdef.h>
#include <iledef.h>
#include <lib$routines.h>
#include <starlet.h>
clock_t get_cpu_time()
{
    clock_t result = (clock_t) 0;
    IOSB iosb;
    int status;
    long cputime = 0;
    ILE3 jpi_items[] = {
        { sizeof( cputime ), JPI$_CPUTIM, &cputime, NULL },
        { 0 },
    };
    status = sys$getjpiw (EFN$C_ENF, 0, 0, jpi_items, &iosb, 0, 0);
    if ( !$VMS_STATUS_SUCCESS( status ) )
    {
        lib$signal( status );
        result = (clock_t) -1;
        return result;
    }
    result = ( cputime / 100 ) * CLOCKS_PER_SEC;
    return result;
}
int32_t shell_maxline()
{
    return MAXLINE;
}
# endif
