#include "cwd.h"
#include "jam.h"
#include "mem.h"
#include "output.h"
#include "pathsys.h"
#include "startup.h"
#include <assert.h>
#include <errno.h>
#include <limits.h>
#if defined( NT ) && !defined( __GNUC__ )
# include <direct.h>
# define PATH_MAX _MAX_PATH
#else
# include <unistd.h>
# if defined( __COMO__ )
#  include <linux/limits.h>
# endif
#endif
#ifndef PATH_MAX
# define PATH_MAX 1024
#endif
static OBJECT * cwd_;
namespace
{
    std::string cwd_s;
}
void cwd_init( void )
{
    int buffer_size = PATH_MAX;
    char * cwd_buffer = 0;
    int error;
    assert( !cwd_ );
    do
    {
        char * const buffer = (char *)BJAM_MALLOC_RAW( buffer_size );
#ifdef OS_VMS
        cwd_buffer = getcwd( buffer, buffer_size, 0 );
#else
        cwd_buffer = getcwd( buffer, buffer_size );
#endif
        error = errno;
        if ( cwd_buffer )
        {
            OBJECT * cwd = object_new( cwd_buffer );
            cwd_ = path_as_key( cwd );
            object_free( cwd );
            cwd_s = cwd_buffer;
        }
        buffer_size *= 2;
        BJAM_FREE_RAW( buffer );
    }
    while ( !cwd_ && error == ERANGE );
    if ( !cwd_ )
    {
        errno_puts( "can not get current working directory" );
        b2::clean_exit( EXITBAD );
    }
}
OBJECT * cwd( void )
{
    assert( cwd_ );
    return cwd_;
}
void cwd_done( void )
{
    assert( cwd_ );
    object_free( cwd_ );
    cwd_ = NULL;
}
const std::string & b2::cwd_str()
{
    return cwd_s;
}
