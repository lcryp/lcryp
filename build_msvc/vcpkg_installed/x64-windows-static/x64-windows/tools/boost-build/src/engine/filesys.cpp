#include "jam.h"
#include "filesys.h"
#include "lists.h"
#include "object.h"
#include "pathsys.h"
#include "jam_strings.h"
#include "output.h"
#include <assert.h>
#include <sys/stat.h>
void file_dirscan_( file_info_t * const dir, scanback func, void * closure );
int file_collect_dir_content_( file_info_t * const dir );
void file_query_( file_info_t * const );
void file_archivescan_( file_archive_info_t * const archive, archive_scanback func,
                        void * closure );
int file_collect_archive_content_( file_archive_info_t * const archive );
void file_archive_query_( file_archive_info_t * const );
static void file_archivescan_impl( OBJECT * path, archive_scanback func,
                                   void * closure );
static void file_dirscan_impl( OBJECT * dir, scanback func, void * closure );
static void free_file_archive_info( void * xarchive, void * data );
static void free_file_info( void * xfile, void * data );
static void remove_files_atexit( void );
static struct hash * filecache_hash;
static struct hash * archivecache_hash;
file_archive_info_t * file_archive_info( OBJECT * const path, int * found )
{
    OBJECT * path_key = path_as_key( path );
    file_archive_info_t * archive;
    if ( !archivecache_hash )
        archivecache_hash = hashinit( sizeof( file_archive_info_t ),
            "file_archive_info" );
    archive = (file_archive_info_t *)hash_insert( archivecache_hash, path_key,
            found );
    if ( !*found )
    {
        archive->name = path_key;
        archive->file = 0;
        archive->members = FL0;
    }
    else
        object_free( path_key );
    return archive;
}
file_archive_info_t * file_archive_query( OBJECT * path )
{
    int found;
    file_archive_info_t * const archive = file_archive_info( path, &found );
    file_info_t * file = file_query( path );
    if ( !( file && file->is_file ) )
    {
        return 0;
    }
    archive->file = file;
    return archive;
}
void file_archivescan( OBJECT * path, archive_scanback func, void * closure )
{
    PROFILE_ENTER( FILE_ARCHIVESCAN );
    file_archivescan_impl( path, func, closure );
    PROFILE_EXIT( FILE_ARCHIVESCAN );
}
void file_build1( PATHNAME * const f, string * file )
{
    if ( DEBUG_SEARCH )
    {
        out_printf( "build file: " );
        if ( f->f_root.len )
            out_printf( "root = '%.*s' ", f->f_root.len, f->f_root.ptr );
        if ( f->f_dir.len )
            out_printf( "dir = '%.*s' ", f->f_dir.len, f->f_dir.ptr );
        if ( f->f_base.len )
            out_printf( "base = '%.*s' ", f->f_base.len, f->f_base.ptr );
        out_printf( "\n" );
    }
    if ( f->f_grist.len )
    {
        if ( f->f_grist.ptr[ 0 ] != '<' )
            string_push_back( file, '<' );
        string_append_range(
            file, f->f_grist.ptr, f->f_grist.ptr + f->f_grist.len );
        if ( file->value[ file->size - 1 ] != '>' )
            string_push_back( file, '>' );
    }
}
void file_dirscan( OBJECT * dir, scanback func, void * closure )
{
    PROFILE_ENTER( FILE_DIRSCAN );
    file_dirscan_impl( dir, func, closure );
    PROFILE_EXIT( FILE_DIRSCAN );
}
void file_done()
{
    remove_files_atexit();
    if ( filecache_hash )
    {
        hashenumerate( filecache_hash, free_file_info, (void *)0 );
        hashdone( filecache_hash );
    }
    if ( archivecache_hash )
    {
        hashenumerate( archivecache_hash, free_file_archive_info, (void *)0 );
        hashdone( archivecache_hash );
    }
}
file_info_t * file_info( OBJECT * const path, int * found )
{
    OBJECT * path_key = path_as_key( path );
    file_info_t * finfo;
    if ( !filecache_hash )
        filecache_hash = hashinit( sizeof( file_info_t ), "file_info" );
    finfo = (file_info_t *)hash_insert( filecache_hash, path_key, found );
    if ( !*found )
    {
        finfo->name = path_key;
        finfo->files = L0;
    }
    else
        object_free( path_key );
    return finfo;
}
int file_is_file( OBJECT * const path )
{
    file_info_t const * const ff = file_query( path );
    return ff ? ff->is_file : -1;
}
int file_time( OBJECT * const path, timestamp * const time )
{
    file_info_t const * const ff = file_query( path );
    if ( !ff ) return -1;
    timestamp_copy( time, &ff->time );
    return 0;
}
file_info_t * file_query( OBJECT * const path )
{
    int found;
    file_info_t * const ff = file_info( path, &found );
    if ( !found )
    {
        file_query_( ff );
        if ( ff->exists )
        {
            if ( timestamp_empty( &ff->time ) )
                timestamp_init( &ff->time, 1, 0 );
        }
    }
    if ( !ff->exists )
    {
        return 0;
    }
    return ff;
}
#ifndef OS_NT
void file_query_posix_( file_info_t * const info )
{
    struct stat statbuf;
    char const * const pathstr = object_str( info->name );
    char const * const pathspec = *pathstr ? pathstr : ".";
    if ( stat( pathspec, &statbuf ) < 0 )
    {
        info->is_file = 0;
        info->is_dir = 0;
        info->exists = 0;
        timestamp_clear( &info->time );
    }
    else
    {
        info->is_file = statbuf.st_mode & S_IFREG ? 1 : 0;
        info->is_dir = statbuf.st_mode & S_IFDIR ? 1 : 0;
        info->exists = 1;
#if defined(_POSIX_VERSION) && _POSIX_VERSION >= 200809
#if defined(OS_MACOSX)
        timestamp_init( &info->time, statbuf.st_mtimespec.tv_sec, statbuf.st_mtimespec.tv_nsec );
#else
        timestamp_init( &info->time, statbuf.st_mtim.tv_sec, statbuf.st_mtim.tv_nsec );
#endif
#else
        timestamp_init( &info->time, statbuf.st_mtime, 0 );
#endif
    }
}
void file_supported_fmt_resolution( timestamp * const t )
{
#if defined(_POSIX_VERSION) && _POSIX_VERSION >= 200809
    timestamp_init( t, 0, 1 );
#else
    timestamp_init( t, 1, 0 );
#endif
}
#endif
static LIST * files_to_remove = L0;
void file_remove_atexit( OBJECT * const path )
{
    files_to_remove = list_push_back( files_to_remove, object_copy( path ) );
}
static void file_archivescan_impl( OBJECT * path, archive_scanback func, void * closure )
{
    file_archive_info_t * const archive = file_archive_query( path );
    if ( !archive || !archive->file->is_file )
        return;
    if ( filelist_empty( archive->members ) )
    {
        if ( DEBUG_BINDSCAN )
            printf( "scan archive %s\n", object_str( archive->file->name ) );
        if ( file_collect_archive_content_( archive ) < 0 )
            return;
    }
    file_archivescan_( archive, func, closure );
    {
        FILELISTITER iter = filelist_begin( archive->members );
        FILELISTITER const end = filelist_end( archive->members );
        char buf[ MAXJPATH ];
        for ( ; iter != end ; iter = filelist_next( iter ) )
        {
            file_info_t * member_file = filelist_item( iter );
            LIST * symbols = member_file->files;
            sprintf( buf, "%s(%s)",
                object_str( archive->file->name ),
                object_str( member_file->name ) );
            {
                OBJECT * member = object_new( buf );
                (*func)( closure, member, symbols, 1, &member_file->time );
                object_free( member );
            }
        }
    }
}
static void file_dirscan_impl( OBJECT * dir, scanback func, void * closure )
{
    file_info_t * const d = file_query( dir );
    if ( !d || !d->is_dir )
        return;
    if ( list_empty( d->files ) )
    {
        if ( DEBUG_BINDSCAN )
            out_printf( "scan directory %s\n", object_str( d->name ) );
        if ( file_collect_dir_content_( d ) < 0 )
            return;
    }
    file_dirscan_( d, func, closure );
    {
        LISTITER iter = list_begin( d->files );
        LISTITER const end = list_end( d->files );
        for ( ; iter != end; iter = list_next( iter ) )
        {
            OBJECT * const path = list_item( iter );
            file_info_t const * const ffq = file_query( path );
            (*func)( closure, ffq->name, 1  , &ffq->time );
        }
    }
}
static void free_file_archive_info( void * xarchive, void * data )
{
    file_archive_info_t * const archive = (file_archive_info_t *)xarchive;
    if ( archive ) filelist_free( archive->members );
}
static void free_file_info( void * xfile, void * data )
{
    file_info_t * const file = (file_info_t *)xfile;
    object_free( file->name );
    list_free( file->files );
}
static void remove_files_atexit( void )
{
    LISTITER iter = list_begin( files_to_remove );
    LISTITER const end = list_end( files_to_remove );
    for ( ; iter != end; iter = list_next( iter ) )
        remove( object_str( list_item( iter ) ) );
    list_free( files_to_remove );
    files_to_remove = L0;
}
FILELIST * filelist_new( OBJECT * path )
{
    FILELIST * list = b2::jam::make_ptr<FILELIST>();
    memset( list, 0, sizeof( *list ) );
    list->size = 0;
    list->head = 0;
    list->tail = 0;
    return filelist_push_back( list, path );
}
FILELIST * filelist_push_back( FILELIST * list, OBJECT * path )
{
    FILEITEM * item;
    file_info_t * file;
    if ( filelist_empty( list ) )
    {
        list = filelist_new( path );
        return list;
    }
    item = b2::jam::make_ptr<FILEITEM>();
    item->value = b2::jam::make_ptr<file_info_t>();
    file = item->value;
    file->name = path;
    file->files = L0;
    if ( list->tail )
    {
        list->tail->next = item;
    }
    else
    {
        list->head = item;
    }
    list->tail = item;
    list->size++;
    return list;
}
FILELIST * filelist_push_front( FILELIST * list, OBJECT * path )
{
    FILEITEM * item;
    file_info_t * file;
    if ( filelist_empty( list ) )
    {
        list = filelist_new( path );
        return list;
    }
    item = b2::jam::make_ptr<FILEITEM>();
    memset( item, 0, sizeof( *item ) );
    item->value = b2::jam::make_ptr<file_info_t>();
    file = item->value;
    memset( file, 0, sizeof( *file ) );
    file->name = path;
    file->files = L0;
    if ( list->head )
    {
        item->next = list->head;
    }
    else
    {
        list->tail = item;
    }
    list->head = item;
    list->size++;
    return list;
}
FILELIST * filelist_pop_front( FILELIST * list )
{
    FILEITEM * item;
    if ( filelist_empty( list ) ) return list;
    item = list->head;
    if ( item )
    {
        if ( item->value )
        {
            free_file_info( item->value, 0 );
            b2::jam::free_ptr( item->value );
        }
        list->head = item->next;
        list->size--;
        if ( !list->size ) list->tail = list->head;
        b2::jam::free_ptr( item );
    }
    return list;
}
int filelist_length( FILELIST * list )
{
    int result = 0;
    if ( !filelist_empty( list ) ) result = list->size;
    return result;
}
void filelist_free( FILELIST * list )
{
    if ( filelist_empty( list ) ) return;
    while ( filelist_length( list ) ) filelist_pop_front( list );
    b2::jam::free_ptr( list );
}
int filelist_empty( FILELIST * list )
{
    return ( list == FL0 );
}
FILELISTITER filelist_begin( FILELIST * list )
{
    if ( filelist_empty( list )
         || list->head == 0 ) return (FILELISTITER)0;
    return &list->head->value;
}
FILELISTITER filelist_end( FILELIST * list )
{
    return (FILELISTITER)0;
}
FILELISTITER filelist_next( FILELISTITER iter )
{
    if ( iter )
    {
        FILEITEM * item = (FILEITEM *)iter;
        iter = ( item->next ? &item->next->value : (FILELISTITER)0 );
    }
    return iter;
}
file_info_t * filelist_item( FILELISTITER it )
{
    file_info_t * result = (file_info_t *)0;
    if ( it )
    {
        result = (file_info_t *)*it;
    }
    return result;
}
file_info_t * filelist_front(  FILELIST * list )
{
    if ( filelist_empty( list )
         || list->head == 0 ) return (file_info_t *)0;
    return list->head->value;
}
file_info_t * filelist_back(  FILELIST * list )
{
    if ( filelist_empty( list )
         || list->tail == 0 ) return (file_info_t *)0;
    return list->tail->value;
}
