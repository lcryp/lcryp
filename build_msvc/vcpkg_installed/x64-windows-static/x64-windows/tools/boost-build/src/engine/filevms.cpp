#include "jam.h"
#include "filesys.h"
#include "object.h"
#include "pathsys.h"
#include "jam_strings.h"
#ifdef OS_VMS
#include <assert.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#define STRUCT_DIRENT struct dirent
void path_translate_to_os_( char const * f, string * file );
int file_collect_dir_content_( file_info_t * const d )
{
    LIST * files = L0;
    PATHNAME f;
    DIR * dd;
    STRUCT_DIRENT * dirent;
    string path[ 1 ];
    char const * dirstr;
    assert( d );
    assert( d->is_dir );
    assert( list_empty( d->files ) );
    dirstr = object_str( d->name );
    memset( (char *)&f, '\0', sizeof( f ) );
    f.f_dir.ptr = dirstr;
    f.f_dir.len = strlen( dirstr );
    if ( !*dirstr ) dirstr = ".";
    if ( !( dd = opendir( dirstr ) ) )
        return -1;
    string_new( path );
    while ( ( dirent = readdir( dd ) ) )
    {
        OBJECT * name;
        f.f_base.ptr = dirent->d_name
        #ifdef old_sinix
            - 2
        #endif
            ;
        f.f_base.len = strlen( f.f_base.ptr );
        string_truncate( path, 0 );
        path_build( &f, path );
        name = object_new( path->value );
        if ( file_query( name ) )
            files = list_push_back( files, name );
        else
            object_free( name );
    }
    string_free( path );
    closedir( dd );
    d->files = files;
    return 0;
}
void file_dirscan_( file_info_t * const d, scanback func, void * closure )
{
    assert( d );
    assert( d->is_dir );
    if ( !strcmp( object_str( d->name ), "/" ) )
        (*func)( closure, d->name, 1  , &d->time );
}
int file_mkdir( char const * const path )
{
    return mkdir( (char *)path, 0777 );
}
void file_query_( file_info_t * const info )
{
    file_query_posix_( info );
}
#include <descrip.h>
#include <lbrdef.h>
#include <credef.h>
#include <mhddef.h>
#include <lhidef.h>
#include <lib$routines.h>
#include <starlet.h>
#ifdef __cplusplus
extern "C" {
#endif
int lbr$set_module(
    void **,
    unsigned long *,
    struct dsc$descriptor_s *,
    unsigned short *,
    void * );
int lbr$open( void **,
    struct dsc$descriptor_s *,
    void *,
    void *,
    void *,
    void *,
    void * );
int lbr$ini_control(
    void **,
    unsigned long *,
    unsigned long *,
    void * );
int lbr$get_index(
    void **,
    unsigned long * const,
    int (*func)( struct dsc$descriptor_s *, unsigned long *),
    void * );
int lbr$search(
    void **,
    unsigned long * const,
    unsigned short *,
    int (*func)( struct dsc$descriptor_s *, unsigned long *),
    unsigned long *);
int lbr$close(
    void ** );
#ifdef __cplusplus
}
#endif
static void
file_cvttime(
    unsigned int *curtime,
    time_t *unixtime )
{
    static const int32_t divisor = 10000000;
    static unsigned int bastim[2] = { 0x4BEB4000, 0x007C9567 };
    int delta[2], remainder;
    lib$subx( curtime, bastim, delta );
    lib$ediv( &divisor, delta, unixtime, &remainder );
}
static void downcase_inplace( char * p )
{
    for ( ; *p; ++p )
        *p = tolower( *p );
}
static file_archive_info_t * m_archive = NULL;
static file_info_t * m_member_found = NULL;
static void * m_lbr_context = NULL;
static unsigned short * m_rfa_found = NULL;
static const unsigned long LBR_MODINDEX_NUM = 1,
                           LBR_SYMINDEX_NUM = 2;
static unsigned int set_archive_symbol( struct dsc$descriptor_s *symbol,
                                        unsigned long *rfa )
{
    file_info_t * member = m_member_found;
    char buf[ MAXJPATH ] = { 0 };
    strncpy(buf, symbol->dsc$a_pointer, symbol->dsc$w_length);
    buf[ symbol->dsc$w_length ] = 0;
    member->files = list_push_back( member->files, object_new( buf ) );
    return ( 1 );
}
static unsigned int set_archive_member( struct dsc$descriptor_s *module,
                                        unsigned long *rfa )
{
    file_archive_info_t * archive = m_archive;
    static struct dsc$descriptor_s bufdsc =
          {0, DSC$K_DTYPE_T, DSC$K_CLASS_S, NULL};
    struct mhddef *mhd;
    char filename[128] = { 0 };
    char buf[ MAXJPATH ] = { 0 };
    int status;
    time_t library_date;
    register int i;
    register char *p;
    bufdsc.dsc$a_pointer = filename;
    bufdsc.dsc$w_length = sizeof( filename );
    status = lbr$set_module( &m_lbr_context, rfa, &bufdsc,
                 &bufdsc.dsc$w_length, NULL );
    if ( !(status & 1) )
        return ( 1 );
    mhd = (struct mhddef *)filename;
    file_cvttime( &mhd->mhd$l_datim, &library_date );
    for ( i = 0, p = module->dsc$a_pointer; i < module->dsc$w_length; ++i, ++p )
    filename[ i ] = *p;
    filename[ i ] = '\0';
    if ( strcmp( filename, "" ) != 0 )
    {
        file_info_t * member = 0;
        sprintf( buf, "%s.obj", filename );
        downcase_inplace( buf );
        archive->members = filelist_push_back( archive->members, object_new( buf ) );
        member = filelist_back( archive->members );
        member->is_file = 1;
        member->is_dir = 0;
        member->exists = 0;
        timestamp_init( &member->time, (time_t)library_date, 0 );
        m_member_found = member;
        m_rfa_found = rfa;
        status = lbr$search(&m_lbr_context, &LBR_SYMINDEX_NUM, m_rfa_found, set_archive_symbol, NULL);
    }
    return ( 1 );
}
void file_archscan( char const * arch, scanback func, void * closure )
{
    OBJECT * path = object_new( arch );
    file_archive_info_t * archive = file_archive_query( path );
    object_free( path );
    if ( filelist_empty( archive->members ) )
    {
        if ( DEBUG_BINDSCAN )
            out_printf( "scan archive %s\n", object_str( archive->file->name ) );
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
            LIST * symbols = member_file->files;
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
    unsigned short rfa[3];
    static struct dsc$descriptor_s library =
          {0, DSC$K_DTYPE_T, DSC$K_CLASS_S, NULL};
    unsigned long lfunc = LBR$C_READ;
    unsigned long typ = LBR$C_TYP_UNK;
    register int status;
    string buf[ 1 ];
    char vmspath[ MAXJPATH ] = { 0 };
    m_archive = archive;
    if ( ! filelist_empty( archive->members ) ) filelist_free( archive->members );
    string_new( buf );
    path_translate_to_os_( object_str( archive->file->name ), buf );
    strcpy( vmspath, buf->value );
    string_free( buf );
    status = lbr$ini_control( &m_lbr_context, &lfunc, &typ, NULL );
    if ( !( status & 1 ) )
        return -1;
    library.dsc$a_pointer = vmspath;
    library.dsc$w_length = strlen( vmspath );
    status = lbr$open( &m_lbr_context, &library, NULL, NULL, NULL, NULL, NULL );
    if ( !( status & 1 ) )
        return -1;
    status = lbr$get_index( &m_lbr_context, &LBR_MODINDEX_NUM, set_archive_member, NULL );
    if ( !( status & 1 ) )
        return -1;
    (void) lbr$close( &m_lbr_context );
    return 0;
}
#endif
