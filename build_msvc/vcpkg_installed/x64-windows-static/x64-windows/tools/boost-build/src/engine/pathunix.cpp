#include "jam.h"
#ifdef USE_PATHUNIX
#include "pathsys.h"
#include <stdlib.h>
#include <unistd.h>
unsigned long path_get_process_id_( void )
{
    return getpid();
}
void path_get_temp_path_( string * buffer )
{
    char const * t = getenv( "TMPDIR" );
    string_append( buffer, t ? t : "/tmp" );
}
int path_translate_to_os_( char const * f, string * file )
{
    int translated = 0;
    string_copy( file, f );
    return translated;
}
void path_register_key( OBJECT * path )
{
}
OBJECT * path_as_key( OBJECT * path )
{
    return object_copy( path );
}
void path_done( void )
{
}
#endif
