#ifndef EXECCMD_H
#define EXECCMD_H
#include "config.h"
#include "lists.h"
#include "jam_strings.h"
#include "timestamp.h"
typedef struct timing_info
{
    double system;
    double user;
    timestamp start;
    timestamp end;
} timing_info;
typedef void (* ExecCmdCallback)
(
    void * const closure,
    int const status,
    timing_info const * const,
    char const * const cmd_stdout,
    char const * const cmd_stderr,
    int const cmd_exit_reason
);
void exec_init( void );
void exec_done( void );
#define EXEC_CMD_OK    0
#define EXEC_CMD_FAIL  1
#define EXEC_CMD_INTR  2
int exec_check
(
    string const * command,
    LIST * * pShell,
    int32_t * error_length,
    int32_t * error_max_length
);
#define EXEC_CHECK_OK             101
#define EXEC_CHECK_NOOP           102
#define EXEC_CHECK_LINE_TOO_LONG  103
#define EXEC_CHECK_TOO_LONG       104
#define EXEC_CMD_QUIET 1
void exec_cmd
(
    string const * command,
    int flags,
    ExecCmdCallback func,
    void * closure,
    LIST * shell
);
void exec_wait();
void argv_from_shell( char const * * argv, LIST * shell, char const * command,
    int32_t const slot );
void onintr( int disp );
int interrupted( void );
int is_raw_command_request( LIST * shell );
int check_cmd_for_too_long_lines( char const * command, int32_t max,
    int32_t * const error_length, int32_t * const error_max_length );
int32_t shell_maxline();
#endif
