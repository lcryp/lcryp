#ifndef JAM_H_VP_2003_08_01
#define JAM_H_VP_2003_08_01
#include "config.h"
#define HAVE_POPEN 1
#ifdef VMS
#include <types.h>
#include <file.h>
#include <stat.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <unixlib.h>
#define OSMINOR "OS=VMS"
#define OSMAJOR "VMS=true"
#define OS_VMS
#define MAXLINE 1024
#define PATH_DELIM '/'
#define SPLITPATH ','
#define EXITOK EXIT_SUCCESS
#define EXITBAD EXIT_FAILURE
#define DOWNSHIFT_PATHS
#ifndef __DECC
#define OSPLAT "OSPLAT=VAX"
#endif
#define glob jam_glob
#endif
#ifdef NT
#include <ctype.h>
#include <fcntl.h>
#include <malloc.h>
#ifndef __MWERKS__
    #include <memory.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#define OSMAJOR "NT=true"
#define OSMINOR "OS=NT"
#define OS_NT
#define SPLITPATH ';'
#define MAXLINE (undefined__see_execnt_c)
#define USE_EXECNT
#define USE_PATHNT
#define PATH_DELIM '\\'
#ifdef AS400
    #undef OSMINOR
    #undef OSMAJOR
    #define OSMAJOR "AS400=true"
    #define OSMINOR "OS=AS400"
    #define OS_AS400
#endif
#ifdef __MSL__
    #undef HAVE_POPEN
#endif
#endif
#ifdef MINGW
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <malloc.h>
#include <memory.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#define OSMAJOR "MINGW=true"
#define OSMINOR "OS=MINGW"
#define OS_NT
#define SPLITPATH ';'
#define MAXLINE 996
#define USE_EXECUNIX
#define USE_PATHNT
#define PATH_DELIM '\\'
#endif
#ifndef OSMINOR
#define OSMAJOR "UNIX=true"
#define USE_EXECUNIX
#define USE_FILEUNIX
#define USE_PATHUNIX
#define PATH_DELIM '/'
#ifdef _AIX
    #define unix
    #define MAXLINE 23552
    #define OSMINOR "OS=AIX"
    #define OS_AIX
    #define NO_VFORK
#endif
#ifdef AMIGA
    #define OSMINOR "OS=AMIGA"
    #define OS_AMIGA
#endif
#ifdef __BEOS__
    #define unix
    #define OSMINOR "OS=BEOS"
    #define OS_BEOS
    #define NO_VFORK
#endif
#ifdef __bsdi__
    #define OSMINOR "OS=BSDI"
    #define OS_BSDI
#endif
#if defined (COHERENT) && defined (_I386)
    #define OSMINOR "OS=COHERENT"
    #define OS_COHERENT
    #define NO_VFORK
#endif
#if defined(__cygwin__) || defined(__CYGWIN__)
    #define OSMINOR "OS=CYGWIN"
    #define OS_CYGWIN
#endif
#if defined(__FreeBSD__) && !defined(__DragonFly__)
    #define OSMINOR "OS=FREEBSD"
    #define OS_FREEBSD
#endif
#ifdef __DragonFly__
    #define OSMINOR "OS=DRAGONFLYBSD"
    #define OS_DRAGONFLYBSD
#endif
#ifdef __DGUX__
    #define OSMINOR "OS=DGUX"
    #define OS_DGUX
#endif
#ifdef __GNU__
    #define OSMINOR "OS=HURD"
    #define OS_HURD
#endif
#ifdef __hpux
    #define OSMINOR "OS=HPUX"
    #define OS_HPUX
#endif
#ifdef __HAIKU__
    #define unix
    #define OSMINOR "OS=HAIKU"
    #define OS_HAIKU
#endif
#ifdef __OPENNT
    #define unix
    #define OSMINOR "OS=INTERIX"
    #define OS_INTERIX
    #define NO_VFORK
#endif
#ifdef __sgi
    #define OSMINOR "OS=IRIX"
    #define OS_IRIX
    #define NO_VFORK
#endif
#ifdef __ISC
    #define OSMINOR "OS=ISC"
    #define OS_ISC
    #define NO_VFORK
#endif
#if defined(linux) || defined(__linux) || \
    defined(__linux__) || defined(__gnu_linux__)
    #define OSMINOR "OS=LINUX"
    #define OS_LINUX
#endif
#ifdef __Lynx__
    #define OSMINOR "OS=LYNX"
    #define OS_LYNX
    #define NO_VFORK
    #define unix
#endif
#ifdef __MACHTEN__
    #define OSMINOR "OS=MACHTEN"
    #define OS_MACHTEN
#endif
#ifdef mpeix
    #define unix
    #define OSMINOR "OS=MPEIX"
    #define OS_MPEIX
    #define NO_VFORK
#endif
#ifdef __MVS__
    #define unix
    #define OSMINOR "OS=MVS"
    #define OS_MVS
#endif
#ifdef _ATT4
    #define OSMINOR "OS=NCR"
    #define OS_NCR
#endif
#ifdef __NetBSD__
    #define unix
    #define OSMINOR "OS=NETBSD"
    #define OS_NETBSD
    #define NO_VFORK
#endif
#ifdef __QNX__
    #define unix
    #ifdef __QNXNTO__
        #define OSMINOR "OS=QNXNTO"
        #define OS_QNXNTO
    #else
        #define OSMINOR "OS=QNX"
        #define OS_QNX
        #define NO_VFORK
        #define MAXLINE 996
    #endif
#endif
#ifdef NeXT
    #ifdef __APPLE__
        #define OSMINOR "OS=RHAPSODY"
        #define OS_RHAPSODY
    #else
        #define OSMINOR "OS=NEXT"
        #define OS_NEXT
    #endif
#endif
#ifdef __APPLE__
    #define unix
    #define OSMINOR "OS=MACOSX"
    #define OS_MACOSX
#endif
#ifdef __osf__
    #ifndef unix
        #define unix
    #endif
    #define OSMINOR "OS=OSF"
    #define OS_OSF
#endif
#ifdef _SEQUENT_
    #define OSMINOR "OS=PTX"
    #define OS_PTX
#endif
#ifdef M_XENIX
    #define OSMINOR "OS=SCO"
    #define OS_SCO
    #define NO_VFORK
#endif
#ifdef sinix
    #define unix
    #define OSMINOR "OS=SINIX"
    #define OS_SINIX
#endif
#ifdef sun
    #if defined(__svr4__) || defined(__SVR4)
        #define OSMINOR "OS=SOLARIS"
        #define OS_SOLARIS
    #else
        #define OSMINOR "OS=SUNOS"
        #define OS_SUNOS
    #endif
#endif
#ifdef ultrix
    #define OSMINOR "OS=ULTRIX"
    #define OS_ULTRIX
#endif
#ifdef _UNICOS
    #define OSMINOR "OS=UNICOS"
    #define OS_UNICOS
#endif
#if defined(__USLC__) && !defined(M_XENIX)
    #define OSMINOR "OS=UNIXWARE"
    #define OS_UNIXWARE
#endif
#ifdef __OpenBSD__
    #define OSMINOR "OS=OPENBSD"
    #define OS_OPENBSD
    #ifndef unix
        #define unix
    #endif
#endif
#if defined (__FreeBSD_kernel__) && !defined(__FreeBSD__)
    #define OSMINOR "OS=KFREEBSD"
    #define OS_KFREEBSD
#endif
#ifndef OSMINOR
    #define OSMINOR "OS=UNKNOWN"
#endif
#include <sys/types.h>
#ifndef OS_MPEIX
    #include <sys/file.h>
#endif
#include <fcntl.h>
#include <stdio.h>
#include <ctype.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#ifndef OS_QNX
    #include <memory.h>
#endif
#ifndef OS_ULTRIX
    #include <stdlib.h>
#endif
#if !defined( OS_BSDI         ) && \
    !defined( OS_FREEBSD      ) && \
    !defined( OS_DRAGONFLYBSD ) && \
    !defined( OS_NEXT         ) && \
    !defined( OS_MACHTEN      ) && \
    !defined( OS_MACOSX       ) && \
    !defined( OS_RHAPSODY     ) && \
    !defined( OS_MVS          ) && \
    !defined( OS_OPENBSD      )
    #include <malloc.h>
#endif
#endif
#if defined( _M_PPC      ) || \
    defined( PPC         ) || \
    defined( ppc         ) || \
    defined( __powerpc__ ) || \
    defined( __ppc__     )
    #define OSPLAT "OSPLAT=PPC"
#endif
#if defined( _ALPHA_   ) || \
    defined( __alpha__ )
    #define OSPLAT "OSPLAT=AXP"
#endif
#if defined( _i386_   ) || \
    defined( __i386__ ) || \
    defined( __i386   ) || \
    defined( _M_IX86  )
    #define OSPLAT "OSPLAT=X86"
#endif
#if defined( __ia64__ ) || \
    defined( __IA64__ ) || \
    defined( __ia64   )
    #define OSPLAT "OSPLAT=IA64"
#endif
#if defined( __x86_64__ ) || \
    defined( __amd64__  ) || \
    defined( _M_AMD64   )
    #define OSPLAT "OSPLAT=X86_64"
#endif
#if defined( __sparc__ ) || \
    defined( __sparc   )
    #define OSPLAT "OSPLAT=SPARC"
#endif
#ifdef __mips__
  #if _MIPS_SIM == _MIPS_SIM_ABI64
    #define OSPLAT "OSPLAT=MIPS64"
  #elif _MIPS_SIM == _MIPS_SIM_ABI32
    #define OSPLAT "OSPLAT=MIPS32"
  #endif
#endif
#if defined( __arm__ ) || \
    defined( __aarch64__ )
    #define OSPLAT "OSPLAT=ARM"
#endif
#ifdef __s390__
    #define OSPLAT "OSPLAT=390"
#endif
#ifdef __hppa
    #define OSPLAT "OSPLAT=PARISC"
#endif
#ifndef OSPLAT
    #define OSPLAT ""
#endif
#ifndef MAXLINE
    #define MAXLINE 102400
#endif
#ifndef EXITOK
    #define EXITOK 0
    #define EXITBAD 1
#endif
#ifndef SPLITPATH
    #define SPLITPATH ':'
#endif
#define MAXSYM   1024
#define MAXJPATH 1024
#define MAXARGC  32
#define DEBUG_MAX  14
struct globs
{
    int    noexec;
    int    jobs;
    int    quitquick;
    int    newestfirst;
    int    pipe_action;
    char   debug[ DEBUG_MAX ];
    FILE * out;
    long   timeout;
    int    dart;
    int    max_buf;
};
extern struct globs globs;
#define DEBUG_MAKE     ( globs.debug[ 1 ] )
#define DEBUG_MAKEQ    ( globs.debug[ 2 ] )
#define DEBUG_EXEC     ( globs.debug[ 2 ] )
#define DEBUG_MAKEPROG ( globs.debug[ 3 ] )
#define DEBUG_BIND     ( globs.debug[ 3 ] )
#define DEBUG_EXECCMD  ( globs.debug[ 4 ] )
#define DEBUG_COMPILE  ( globs.debug[ 5 ] )
#define DEBUG_HEADER   ( globs.debug[ 6 ] )
#define DEBUG_BINDSCAN ( globs.debug[ 6 ] )
#define DEBUG_SEARCH   ( globs.debug[ 6 ] )
#define DEBUG_VARSET   ( globs.debug[ 7 ] )
#define DEBUG_VARGET   ( globs.debug[ 8 ] )
#define DEBUG_VAREXP   ( globs.debug[ 8 ] )
#define DEBUG_IF       ( globs.debug[ 8 ] )
#define DEBUG_LISTS    ( globs.debug[ 9 ] )
#define DEBUG_SCAN     ( globs.debug[ 9 ] )
#define DEBUG_MEM      ( globs.debug[ 9 ] )
#define DEBUG_PROFILE  ( globs.debug[ 10 ] )
#define DEBUG_PARSE    ( globs.debug[ 11 ] )
#define DEBUG_GRAPH    ( globs.debug[ 12 ] )
#define DEBUG_FATE     ( globs.debug[ 13 ] )
#include "mem.h"
#include "debug.h"
#endif
