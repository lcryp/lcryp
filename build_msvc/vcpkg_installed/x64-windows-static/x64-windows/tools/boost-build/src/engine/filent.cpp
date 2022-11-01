#include "jam.h"
#ifdef OS_NT
#include "filesys.h"
#include "object.h"
#include "pathsys.h"
#include "jam_strings.h"
#include "output.h"
#ifdef __BORLANDC__
# undef FILENAME
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <assert.h>
#include <ctype.h>
#include <direct.h>
#include <io.h>
int file_collect_archive_content_( file_archive_info_t * const archive );
int file_collect_dir_content_( file_info_t * const d )
{
    PATHNAME f;
    string pathspec[ 1 ];
    string pathname[ 1 ];
    LIST * files = L0;
    int32_t d_length;
    assert( d );
    assert( d->is_dir );
    assert( list_empty( d->files ) );
    d_length = int32_t(strlen( object_str( d->name ) ));
    memset( (char *)&f, '\0', sizeof( f ) );
    f.f_dir.ptr = object_str( d->name );
    f.f_dir.len = d_length;
    if ( !d_length )
        string_copy( pathspec, ".\\*" );
    else
    {
        char const trailingChar = object_str( d->name )[ d_length - 1 ] ;
        string_copy( pathspec, object_str( d->name ) );
        if ( ( trailingChar != '\\' ) && ( trailingChar != '/' ) )
            string_append( pathspec, "\\" );
        string_append( pathspec, "*" );
    }
    {
        WIN32_FIND_DATAA finfo;
        HANDLE const findHandle = FindFirstFileA( pathspec->value, &finfo );
        if ( findHandle == INVALID_HANDLE_VALUE )
        {
            string_free( pathspec );
            return -1;
        }
        string_new( pathname );
        do
        {
            OBJECT * pathname_obj;
            f.f_base.ptr = finfo.cFileName;
            f.f_base.len = int32_t(strlen( finfo.cFileName ));
            string_truncate( pathname, 0 );
            path_build( &f, pathname );
            pathname_obj = object_new( pathname->value );
            path_register_key( pathname_obj );
            files = list_push_back( files, pathname_obj );
            {
                int found;
                file_info_t * const ff = file_info( pathname_obj, &found );
                ff->is_dir = finfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
                ff->is_file = !ff->is_dir;
                ff->exists = 1;
                timestamp_from_filetime( &ff->time, &finfo.ftLastWriteTime );
                if ( finfo.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT )
                {
                    HANDLE hLink = CreateFileA( pathname->value, 0, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL );
                    BY_HANDLE_FILE_INFORMATION target_finfo[ 1 ];
                    if ( hLink != INVALID_HANDLE_VALUE && GetFileInformationByHandle( hLink, target_finfo ) )
                    {
                        ff->is_file = target_finfo->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ? 0 : 1;
                        ff->is_dir = target_finfo->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ? 1 : 0;
                        timestamp_from_filetime( &ff->time, &target_finfo->ftLastWriteTime );
                    }
                }
            }
        }
        while ( FindNextFileA( findHandle, &finfo ) );
        FindClose( findHandle );
    }
    string_free( pathname );
    string_free( pathspec );
    d->files = files;
    return 0;
}
void file_dirscan_( file_info_t * const d, scanback func, void * closure )
{
    assert( d );
    assert( d->is_dir );
    {
        char const * const name = object_str( d->name );
        if ( name[ 0 ] == '\\' && !name[ 1 ] )
        {
            (*func)( closure, d->name, 1  , &d->time );
        }
        else if ( name[ 0 ] && name[ 1 ] == ':' && name[ 2 ] && !name[ 3 ] )
        {
            OBJECT * dir_no_slash = object_new_range( name, 2 );
            (*func)( closure, d->name, 1  , &d->time );
            (*func)( closure, dir_no_slash, 1  , &d->time );
            object_free( dir_no_slash );
        }
    }
}
int file_mkdir( char const * const path )
{
    return _mkdir( path );
}
int try_file_query_root( file_info_t * const info )
{
    WIN32_FILE_ATTRIBUTE_DATA fileData;
    char buf[ 4 ];
    char const * const pathstr = object_str( info->name );
    if ( !pathstr[ 0 ] )
    {
        buf[ 0 ] = '.';
        buf[ 1 ] = 0;
    }
    else if ( pathstr[ 0 ] == '\\' && ! pathstr[ 1 ] )
    {
        buf[ 0 ] = '\\';
        buf[ 1 ] = '\0';
    }
    else if ( pathstr[ 1 ] == ':' )
    {
        if ( !pathstr[ 2 ] || ( pathstr[ 2 ] == '\\' && !pathstr[ 3 ] ) )
        {
            buf[ 0 ] = pathstr[ 0 ];
            buf[ 1 ] = ':';
            buf[ 2 ] = '\\';
            buf[ 3 ] = '\0';
        }
        else
        {
            return 0;
        }
    }
    else
    {
        return 0;
    }
    if ( !GetFileAttributesExA( buf, GetFileExInfoStandard, &fileData ) )
    {
        info->is_dir = 0;
        info->is_file = 0;
        info->exists = 0;
        timestamp_clear( &info->time );
    }
    else
    {
        info->is_dir = fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
        info->is_file = !info->is_dir;
        info->exists = 1;
        timestamp_from_filetime( &info->time, &fileData.ftLastWriteTime );
    }
    return 1;
}
void file_query_( file_info_t * const info )
{
    char const * const pathstr = object_str( info->name );
    const char * dir;
    OBJECT * parent;
    file_info_t * parent_info;
    if ( try_file_query_root( info ) )
        return;
    if ( ( dir = strrchr( pathstr, '\\' ) ) )
    {
        parent = object_new_range( pathstr, int32_t(dir - pathstr) );
    }
    else
    {
        parent = object_copy( constant_empty );
    }
    parent_info = file_query( parent );
    object_free( parent );
    if ( !parent_info || !parent_info->is_dir )
    {
        info->is_dir = 0;
        info->is_file = 0;
        info->exists = 0;
        timestamp_clear( &info->time );
    }
    else
    {
        info->is_dir = 0;
        info->is_file = 0;
        info->exists = 0;
        timestamp_clear( &info->time );
        if ( list_empty( parent_info->files ) )
            file_collect_dir_content_( parent_info );
    }
}
void file_supported_fmt_resolution( timestamp * const t )
{
    timestamp_init( t, 0, 0 );
}
#define ARMAG  "!<arch>\n"
#define SARMAG  8
#define ARFMAG  "`\n"
struct ar_hdr
{
    char ar_name[ 16 ];
    char ar_date[ 12 ];
    char ar_uid[ 6 ];
    char ar_gid[ 6 ];
    char ar_mode[ 8 ];
    char ar_size[ 10 ];
    char ar_fmag[ 2 ];
};
#define SARFMAG  2
#define SARHDR  sizeof( struct ar_hdr )
void file_archscan( char const * arch, scanback func, void * closure )
{
    OBJECT * path = object_new( arch );
    file_archive_info_t * archive = file_archive_query( path );
    object_free( path );
    if ( filelist_empty( archive->members ) )
    {
        if ( file_collect_archive_content_( archive ) < 0 )
            return;
    }
    {
        FILELISTITER iter = filelist_begin( archive->members );
        FILELISTITER const end = filelist_end( archive->members );
        char buf[ MAXJPATH ];
        for ( ; iter != end ; iter = filelist_next( iter ) )
        {
            file_info_t * member_file = filelist_item( iter );
            sprintf( buf, "%s(%s)",
                object_str( archive->file->name ),
                object_str( member_file->name ) );
            {
                OBJECT * member = object_new( buf );
                (*func)( closure, member, 1  , &member_file->time );
                object_free( member );
            }
        }
    }
}
void file_archivescan_( file_archive_info_t * const archive, archive_scanback func,
                        void * closure )
{
}
int file_collect_archive_content_( file_archive_info_t * const archive )
{
    struct ar_hdr ar_hdr;
    char * string_table = 0;
    char buf[ MAXJPATH ];
    long offset;
    const char * path = object_str( archive->file->name );
    int const fd = open( path , O_RDONLY | O_BINARY, 0 );
    if ( ! filelist_empty( archive->members ) ) filelist_free( archive->members );
    if ( fd < 0 )
        return -1;
    if ( read( fd, buf, SARMAG ) != SARMAG || strncmp( ARMAG, buf, SARMAG ) )
    {
        close( fd );
        return -1;
    }
    offset = SARMAG;
    if ( DEBUG_BINDSCAN )
        out_printf( "scan archive %s\n", path );
    while ( ( read( fd, &ar_hdr, SARHDR ) == SARHDR ) &&
        !memcmp( ar_hdr.ar_fmag, ARFMAG, SARFMAG ) )
    {
        long lar_date;
        long lar_size;
        char * name = 0;
        char * endname;
        sscanf( ar_hdr.ar_date, "%ld", &lar_date );
        sscanf( ar_hdr.ar_size, "%ld", &lar_size );
        lar_size = ( lar_size + 1 ) & ~1;
        if ( ar_hdr.ar_name[ 0 ] == '/' && ar_hdr.ar_name[ 1 ] == '/' )
        {
            string_table = (char*)BJAM_MALLOC_ATOMIC( lar_size + 1 );
            if ( read( fd, string_table, lar_size ) != lar_size )
                out_printf( "error reading string table\n" );
            string_table[ lar_size ] = '\0';
            offset += SARHDR + lar_size;
            continue;
        }
        else if ( ar_hdr.ar_name[ 0 ] == '/' && ar_hdr.ar_name[ 1 ] != ' ' )
        {
            name = string_table + atoi( ar_hdr.ar_name + 1 );
            for ( endname = name; *endname && *endname != '\n'; ++endname );
        }
        else
        {
            name = ar_hdr.ar_name;
            endname = name + sizeof( ar_hdr.ar_name );
        }
        while ( endname-- > name )
            if ( !isspace( *endname ) && ( *endname != '\\' ) && ( *endname !=
                '/' ) )
                break;
        *++endname = 0;
        {
            char * c;
            if ( (c = strrchr( name, '/' )) != nullptr )
                name = c + 1;
            if ( (c = strrchr( name, '\\' )) != nullptr )
                name = c + 1;
        }
        sprintf( buf, "%.*s", int(endname - name), name );
        if ( strcmp( buf, "") != 0 )
        {
            file_info_t * member = 0;
            archive->members = filelist_push_front( archive->members, object_new( buf ) );
            member = filelist_front( archive->members );
            member->is_file = 1;
            member->is_dir = 0;
            member->exists = 0;
            timestamp_init( &member->time, (time_t)lar_date, 0 );
        }
        offset += SARHDR + lar_size;
        lseek( fd, offset, 0 );
    }
    close( fd );
    return 0;
}
#endif
