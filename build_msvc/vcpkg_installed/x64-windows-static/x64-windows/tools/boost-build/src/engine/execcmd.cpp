#include "jam.h"
#include "execcmd.h"
#include "output.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static int intr;
void argv_from_shell( char const * * argv, LIST * shell, char const * command,
    int32_t const slot )
{
    static char jobno[ 12 ];
    int i;
    int gotpercent = 0;
    LISTITER iter = list_begin( shell );
    LISTITER end = list_end( shell );
    assert( 0 <= slot );
    assert( slot < 999 );
    sprintf( jobno, "%d", slot + 1 );
    for ( i = 0; iter != end && i < MAXARGC; ++i, iter = list_next( iter ) )
    {
        switch ( object_str( list_item( iter ) )[ 0 ] )
        {
            case '%': argv[ i ] = command; ++gotpercent; break;
            case '!': argv[ i ] = jobno; break;
            default : argv[ i ] = object_str( list_item( iter ) );
        }
    }
    if ( !gotpercent )
        argv[ i++ ] = command;
    argv[ i ] = NULL;
}
int check_cmd_for_too_long_lines( char const * command, int32_t max,
    int32_t * const error_length, int32_t * const error_max_length )
{
    while ( *command )
    {
        int32_t const l = int32_t(strcspn( command, "\n" ));
        if ( l > max )
        {
            *error_length = l;
            *error_max_length = max;
            return EXEC_CHECK_LINE_TOO_LONG;
        }
        command += l;
        if ( *command )
            ++command;
    }
    return EXEC_CHECK_OK;
}
int is_raw_command_request( LIST * shell )
{
    return !list_empty( shell ) &&
        !strcmp( object_str( list_front( shell ) ), "%" ) &&
        list_next( list_begin( shell ) ) == list_end( shell );
}
int interrupted( void )
{
    return intr != 0;
}
void onintr( int disp )
{
    ++intr;
    out_printf( "...interrupted\n" );
}
