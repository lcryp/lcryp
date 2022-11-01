#ifndef PATHSYS_VP_20020211_H
#define PATHSYS_VP_20020211_H
#include "config.h"
#include "object.h"
#include "jam_strings.h"
#include <string>
typedef struct _pathpart
{
    char const * ptr;
    int32_t len;
} PATHPART;
typedef struct _pathname
{
    PATHPART part[ 6 ];
#define f_grist   part[ 0 ]
#define f_root    part[ 1 ]
#define f_dir     part[ 2 ]
#define f_base    part[ 3 ]
#define f_suffix  part[ 4 ]
#define f_member  part[ 5 ]
} PATHNAME;
void path_build( PATHNAME *, string * file );
void path_parse( char const * file, PATHNAME * );
void path_parent( PATHNAME * );
int path_translate_to_os( char const *, string * file );
OBJECT * path_as_key( OBJECT * path );
void path_register_key( OBJECT * canonic_path );
string const * path_tmpdir( void );
OBJECT * path_tmpnam( void );
OBJECT * path_tmpfile( void );
char * executable_path( char const * argv0 );
void path_done( void );
namespace b2
{
    namespace paths
    {
        inline bool is_rooted(const std::string &p)
        {
            #if NT
            return
                (p.size() >= 1 && (p[0] == '/' || p[0] == '\\')) ||
                (p.size() >= 3 && p[1] == ':' && (p[2] == '/' || p[2] == '\\'));
            #else
            return
                (p.size() >= 1 && (p[0] == '/' || p[0] == '\\'));
            #endif
        }
        inline bool is_relative(const std::string &p)
        {
            return
                (p.size() >= 3 && (
                    (p[0] == '.' && p[1] == '.' && (p[2] == '/' || p[2] == '\\')) ||
                    (p[0] == '.' && (p[1] == '/' || p[1] == '\\'))
                    ));
        }
        std::string normalize(const std::string &p);
    }
}
#endif
