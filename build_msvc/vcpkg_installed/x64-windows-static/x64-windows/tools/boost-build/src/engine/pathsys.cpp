#include "jam.h"
#include "cwd.h"
#include "filesys.h"
#include "pathsys.h"
#include <stdlib.h>
#include <time.h>
#include <algorithm>
unsigned long path_get_process_id_( void );
void path_get_temp_path_( string * buffer );
int path_translate_to_os_( char const * f, string * file );
void path_parse( char const * file, PATHNAME * f )
{
    char const * p;
    char const * q;
    char const * end;
    memset( (char *)f, 0, sizeof( *f ) );
    if ( ( file[ 0 ] == '<' ) && ( p = strchr( file, '>' ) ) )
    {
        f->f_grist.ptr = file;
        f->f_grist.len = int32_t(p - file);
        file = p + 1;
    }
    p = strrchr( file, '/' );
#if PATH_DELIM == '\\'
    {
        char const * p1 = strrchr( p ? p + 1 : file, '\\' );
        if ( p1 ) p = p1;
    }
#endif
    if ( p )
    {
        f->f_dir.ptr = file;
        f->f_dir.len = int32_t(p - file);
        if ( !f->f_dir.len )
            ++f->f_dir.len;
#if PATH_DELIM == '\\'
        if ( f->f_dir.len == 2 && file[ 1 ] == ':' )
            ++f->f_dir.len;
#endif
        file = p + 1;
    }
    end = file + strlen( file );
    if ( ( p = strchr( file, '(' ) ) && ( end[ -1 ] == ')' ) )
    {
        f->f_member.ptr = p + 1;
        f->f_member.len = int32_t(end - p - 2);
        end = p;
    }
    p = 0;
    for ( q = file; ( q = (char *)memchr( q, '.', end - q ) ); ++q )
        p = q;
    if ( p )
    {
        f->f_suffix.ptr = p;
        f->f_suffix.len = int32_t(end - p);
        end = p;
    }
    f->f_base.ptr = file;
    f->f_base.len = int32_t(end - file);
}
static int is_path_delim( char const c )
{
    return c == PATH_DELIM
#if PATH_DELIM == '\\'
        || c == '/'
#endif
        ;
}
static char as_path_delim( char const c )
{
    return is_path_delim( c ) ? c : PATH_DELIM;
}
void path_build( PATHNAME * f, string * file )
{
    int check_f;
    int check_f_pos;
    file_build1( f, file );
    check_f = (f->f_root.len
               && !( f->f_root.len == 1 && f->f_root.ptr[ 0 ] == '.')
               && !( f->f_dir.len && f->f_dir.ptr[ 0 ] == '/' ));
#if PATH_DELIM == '\\'
    check_f = (check_f
               && !( f->f_dir.len && f->f_dir.ptr[ 0 ] == '\\' )
               && !( f->f_dir.len && f->f_dir.ptr[ 1 ] == ':' ));
#endif
    if (check_f)
    {
        string_append_range( file, f->f_root.ptr, f->f_root.ptr + f->f_root.len
            );
        if ( !is_path_delim( f->f_root.ptr[ f->f_root.len - 1 ] ) )
            string_push_back( file, as_path_delim( f->f_root.ptr[ f->f_root.len
                ] ) );
    }
    if ( f->f_dir.len )
        string_append_range( file, f->f_dir.ptr, f->f_dir.ptr + f->f_dir.len );
    check_f_pos = (f->f_dir.len && ( f->f_base.len || f->f_suffix.len ));
#if PATH_DELIM == '\\'
    check_f_pos = (check_f_pos && !( f->f_dir.len == 3 && f->f_dir.ptr[ 1 ] == ':' ));
#endif
    check_f_pos = (check_f_pos && !( f->f_dir.len == 1 && is_path_delim( f->f_dir.ptr[ 0 ])));
    if (check_f_pos)
        string_push_back( file, as_path_delim( f->f_dir.ptr[ f->f_dir.len ] ) );
    if ( f->f_base.len )
        string_append_range( file, f->f_base.ptr, f->f_base.ptr + f->f_base.len
            );
    if ( f->f_suffix.len )
        string_append_range( file, f->f_suffix.ptr, f->f_suffix.ptr +
            f->f_suffix.len );
    if ( f->f_member.len )
    {
        string_push_back( file, '(' );
        string_append_range( file, f->f_member.ptr, f->f_member.ptr +
            f->f_member.len );
        string_push_back( file, ')' );
    }
}
void path_parent( PATHNAME * f )
{
    f->f_base.ptr = f->f_suffix.ptr = f->f_member.ptr = "";
    f->f_base.len = f->f_suffix.len = f->f_member.len = 0;
}
string const * path_tmpdir()
{
    static string buffer[ 1 ];
    static int have_result;
    if ( !have_result )
    {
        string_new( buffer );
        path_get_temp_path_( buffer );
        have_result = 1;
    }
    return buffer;
}
OBJECT * path_tmpnam( void )
{
    char name_buffer[ 64 ];
    unsigned long const pid = path_get_process_id_();
    static unsigned long t;
    if ( !t ) t = time( 0 ) & 0xffff;
    t += 1;
    sprintf( name_buffer, "jam%lx%lx.000", pid, t );
    return object_new( name_buffer );
}
OBJECT * path_tmpfile( void )
{
    OBJECT * result;
    OBJECT * tmpnam;
    string file_path[ 1 ];
    string_copy( file_path, path_tmpdir()->value );
    string_push_back( file_path, PATH_DELIM );
    tmpnam = path_tmpnam();
    string_append( file_path, object_str( tmpnam ) );
    object_free( tmpnam );
    result = object_new( file_path->value );
    string_free( file_path );
    return result;
}
int path_translate_to_os( char const * f, string * file )
{
  return path_translate_to_os_( f, file );
}
std::string b2::paths::normalize(const std::string &p)
{
    std::string result{"/"};
    bool is_rooted = p[0] == '/' || p[0] == '\\';
    result += p;
    std::replace(result.begin(), result.end(), '\\', '/');
    int32_t ellipsis = 0;
    for (auto end_pos = result.length(); end_pos > 0; )
    {
        auto path_pos = result.rfind('/', end_pos-1);
        if (path_pos == std::string::npos) break;
        if (path_pos == end_pos-1)
        {
            result.erase(path_pos, 1);
        }
        else if ((end_pos-path_pos == 2) && result[path_pos+1] == '.')
        {
            result.erase(path_pos, 2);
        }
        else if ((end_pos-path_pos == 3) && result[path_pos+1] == '.' && result[path_pos+2] == '.')
        {
            result.erase(path_pos, 3);
            ellipsis += 1;
        }
        else if (ellipsis > 0)
        {
            result.erase(path_pos, end_pos-path_pos);
            ellipsis -= 1;
        }
        end_pos = path_pos;
    }
    if (ellipsis > 0)
    {
        if (is_rooted) return "";
        do result.insert(0, "/.."); while (--ellipsis > 0);
    }
    if (result.empty()) return is_rooted ? "/" : ".";
    if (!is_rooted) return result.substr(1);
    return result;
}
#if defined(_WIN32)
# define WIN32_LEAN_AND_MEAN
# include <windows.h>
char * executable_path( char const * argv0 )
{
    char buf[ 1024 ];
    DWORD const ret = GetModuleFileNameA( NULL, buf, sizeof( buf ) );
    return ( !ret || ret == sizeof( buf ) ) ? NULL : strdup( buf );
}
#elif defined(__APPLE__)
# include <mach-o/dyld.h>
char *executable_path( char const * argv0 )
{
    char buf[ 1024 ];
    uint32_t size = sizeof( buf );
    return _NSGetExecutablePath( buf, &size ) ? NULL : strdup( buf );
}
#elif defined(sun) || defined(__sun)
# include <stdlib.h>
char * executable_path( char const * argv0 )
{
    const char * execname = getexecname();
    return execname ? strdup( execname ) : NULL;
}
#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
# include <sys/sysctl.h>
char * executable_path( char const * argv0 )
{
    int mib[ 4 ] = { CTL_KERN, KERN_PROC, KERN_PROC_PATHNAME, -1 };
    char buf[ 1024 ];
    size_t size = sizeof( buf );
    sysctl( mib, 4, buf, &size, NULL, 0 );
    return ( !size || size == sizeof( buf ) ) ? NULL : strndup( buf, size );
}
#elif defined(__linux__) || defined(__CYGWIN__) || defined(__GNU__)
# include <unistd.h>
char * executable_path( char const * argv0 )
{
    char buf[ 1024 ];
    ssize_t const ret = readlink( "/proc/self/exe", buf, sizeof( buf ) );
    return ( !ret || ret == sizeof( buf ) ) ? NULL : strndup( buf, ret );
}
#elif defined(OS_VMS)
# include <unixlib.h>
char * executable_path( char const * argv0 )
{
    char * vms_path = NULL;
    char * posix_path = NULL;
    char * p;
    vms_path = strdup( argv0 );
    if ( vms_path && ( p = strchr( vms_path, ';') ) ) *p = '\0';
    posix_path = decc$translate_vms( vms_path );
    if ( vms_path ) free( vms_path );
    return posix_path > 0 ? strdup( posix_path ) : NULL;
}
#else
char * executable_path( char const * argv0 )
{
    char * result = nullptr;
    if (!result && b2::paths::is_rooted(argv0))
        result = strdup( argv0 );
    if (!result && b2::paths::is_relative(argv0))
    {
        auto p = b2::paths::normalize(b2::cwd_str()+"/"+argv0);
        result = strdup( p.c_str() );
    }
    if (!result)
    {
        std::string path_env = getenv( "PATH" );
        std::string::size_type i = 0;
        while (i != std::string::npos)
        {
            std::string::size_type e = path_env.find_first_of(':', i);
            std::string p = e == std::string::npos
                ? path_env.substr(i)
                : path_env.substr(i, e-i);
            if (b2::filesys::is_file(p+"/"+argv0))
            {
                result = strdup( (p+"/"+argv0).c_str() );
                break;
            }
            i = e == std::string::npos ? e : e+1;
        }
    }
    return result;
}
#endif
