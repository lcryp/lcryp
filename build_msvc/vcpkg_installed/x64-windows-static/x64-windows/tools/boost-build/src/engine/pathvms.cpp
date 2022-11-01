#include "jam.h"
#ifdef OS_VMS
#include "pathsys.h"
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <unixlib.h>
unsigned long path_get_process_id_( void )
{
    return getpid();
}
void path_get_temp_path_( string * buffer )
{
    char const * t = getenv( "TMPDIR" );
    string_append( buffer, t ? t : "/tmp" );
}
static string * m_vmsfilespec = NULL;
static int copy_vmsfilespec( char * f, int type )
{
  assert ( NULL != m_vmsfilespec && "Must be bound to a valid object" );
  string_copy( m_vmsfilespec, f );
  return 0;
}
static int translate_path_posix2vms( string * path )
{
    int translated = 0;
    string as_dir[ 1 ];
    string as_file[ 1 ];
    int dir_count;
    int file_count;
    unsigned char is_dir;
    unsigned char is_file;
    unsigned char is_ambiguous;
    string_new( as_dir );
    string_new( as_file );
    m_vmsfilespec = as_dir;
    dir_count = decc$to_vms( path->value, copy_vmsfilespec, 0, 2 );
    m_vmsfilespec = as_file;
    file_count = decc$to_vms( path->value, copy_vmsfilespec, 0, 0 );
    m_vmsfilespec = NULL;
    translated = ( file_count || dir_count );
    if ( file_count && dir_count )
    {
        struct stat statbuf;
        if ( stat(as_dir->value, &statbuf ) < 0
             && stat(as_file->value, &statbuf ) > 0
             && ( statbuf.st_mode & S_IFREG ) )
        {
            string_truncate( path, 0 );
            string_append( path, as_file->value );
        }
        else
        {
            string_truncate( path, 0 );
            string_append( path, as_dir->value );
        }
    }
    else if ( file_count )
    {
        string_truncate( path, 0 );
        string_append( path, as_file->value );
    }
    else if ( dir_count  )
    {
        string_truncate( path, 0 );
        string_append( path, as_dir->value );
    }
    else
    {
        translated = 0;
    }
    string_free( as_dir );
    string_free( as_file );
    return translated;
}
int path_translate_to_os_( char const * f, string * file )
{
    int translated = 0;
    string_copy( file, f );
    translated = translate_path_posix2vms( file );
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
