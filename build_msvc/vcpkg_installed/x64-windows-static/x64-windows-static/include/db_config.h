#define DB_WIN32 1
#if defined(_DEBUG)
#if !defined(DEBUG)
#define DEBUG 1
#endif
#endif
#define HAVE_64BIT_TYPES 1
#define HAVE_ABORT 1
#define HAVE_ATOI 1
#define HAVE_ATOL 1
#define HAVE_ATOMIC_SUPPORT 1
#ifndef HAVE_SMALLBUILD
#define HAVE_COMPRESSION 1
#endif
#ifndef HAVE_SMALLBUILD
#endif
#define HAVE_EXIT_SUCCESS 1
#define HAVE_FCLOSE 1
#define HAVE_FGETC 1
#define HAVE_FGETS 1
#define HAVE_FILESYSTEM_NOTZERO 1
#define HAVE_FOPEN 1
#define HAVE_FTRUNCATE 1
#define HAVE_FWRITE 1
#define HAVE_GETCWD 1
#define HAVE_GETENV 1
#define	HAVE_GETOPT 1
#define HAVE_GETOPT_OPTRESET 1
#ifndef HAVE_SMALLBUILD
#define HAVE_HASH 1
#endif
#define HAVE_ISALPHA 1
#define HAVE_ISDIGIT 1
#define HAVE_ISPRINT 1
#define HAVE_LOCALTIME 1
#define HAVE_ISSPACE 1
#define HAVE_MEMCMP 1
#define HAVE_MEMCPY 1
#define HAVE_MEMMOVE 1
#define HAVE_MEMORY_H 1
#define HAVE_MUTEX_SUPPORT 1
#define HAVE_MUTEX_WIN32 1
#define HAVE_PARTITION 1
#define HAVE_PRINTF 1
#define HAVE_QSORT 1
#ifndef HAVE_SMALLBUILD
#define HAVE_QUEUE 1
#endif
#define HAVE_RAISE 1
#define HAVE_RAND 1
#ifndef HAVE_SMALLBUILD
#define HAVE_REPLICATION 1
#endif
#ifndef HAVE_SMALLBUILD
#define HAVE_REPLICATION_THREADS 1
#endif
#define HAVE_SETUID 1
#define HAVE_SHARED_LATCHES 1
#define HAVE_SIMPLE_THREAD_TYPE 1
#define HAVE_SNPRINTF 1
#define HAVE_STAT 1
#define HAVE_STATISTICS 1
#define HAVE_STDLIB_H 1
#define HAVE_STRCASECMP 1
#define HAVE_STRCAT 1
#define HAVE_STRCHR 1
#define HAVE_STRDUP 1
#define HAVE_STRERROR 1
#define HAVE_STRFTIME 1
#define HAVE_STRINGS_H 1
#define HAVE_STRING_H 1
#define HAVE_STRNCAT 1
#define HAVE_STRNCMP 1
#define HAVE_STRRCHR 1
#define HAVE_STRTOL 1
#define HAVE_STRTOUL 1
#define HAVE_SYS_STAT_H 1
#define HAVE_SYS_TYPES_H 1
#define HAVE_TIME 1
#define HAVE_UPGRADE_SUPPORT 1
#ifndef HAVE_SMALLBUILD
#define HAVE_VERIFY 1
#endif
#define HAVE_VSNPRINTF 1
#define HAVE__FSTATI64 1
#define PACKAGE_BUGREPORT "Oracle Technology Network Berkeley DB forum"
#define PACKAGE_NAME "Berkeley DB"
#define PACKAGE_STRING "Berkeley DB 4.8.30"
#define PACKAGE_TARNAME "db-4.8.30"
#define PACKAGE_VERSION "4.8.30"
#define STDC_HEADERS 1
#ifndef __cplusplus
#define inline __inline
#endif
