#ifndef _DB_STL_COMMON_H
#define _DB_STL_COMMON_H
#ifdef DBSTL_DEBUG_LEAK
#include "vld.h"
#endif
#include <assert.h>
#include "db_cxx.h"
#if !defined(DEBUG) && !defined(_DEBUG)
#undef dbstl_assert
#define dbstl_assert(expression)
#else
#undef dbstl_assert
#define dbstl_assert(expression) do {			\
	if (!(expression)) {				\
		FailedAssertionException ex(__FILE__, __LINE__, #expression);\
		throw ex; } } while (0)
#endif
#ifndef SIZE_T_MAX
#define SIZE_T_MAX 1073741824
#endif
#if defined( DB_WIN32) || defined(_WIN32)
#include <windows.h>
#include <tchar.h>
#else
#define TCHAR char
#define _T(e) (e)
#define _ftprintf fprintf
#define _snprintf snprintf
#define _tcschr strchr
#define _tcscmp strcmp
#define _tcscpy strcpy
#define _tcslen strlen
#define _tgetopt getopt
#define _tmain main
#define _tprintf printf
#define _ttoi atoi
#endif
#define	HAVE_WSTRING	1
#define	TLS_DECL_MODIFIER	__declspec(thread)
#define	TLS_DEFN_MODIFIER	__declspec(thread)
#if !defined(TLS_DECL_MODIFIER) && !defined(HAVE_PTHREAD_TLS)
#error "No appropriate TLS modifier defined."
#endif
#define HAVE_EXPLICIT_KEYWORD			1
#define HAVE_NAMESPACE				1
#define HAVE_TYPENAME				1
#ifdef WIN32
#define CLS_SCOPE(clstmpl_name)
#else
#define CLS_SCOPE(clstmpl_name) clstmpl_name::
#define NO_IN_CLASS_FULL_SPECIALIZATION  1
#define NO_MEMBER_FUNCTION_PARTIAL_SPECIALIZATION 1
#endif
#if HAVE_NAMESPACE
#define START_NS(nsname) namespace nsname {
#define END_NS }
#else
#define START_NS(nsname) struct nsname {
#define END_NS };
#endif
#if HAVE_EXPLICIT_KEYWORD
#define EXPLICIT explicit
#else
#define EXPLICIT
#endif
#if HAVE_TYPENAME
#define Typename typename
#else
#define Typename class
#endif
#define INVALID_ITERATOR_POSITION -1
#define INVALID_ITERATOR_CURSOR -2
#define ITERATOR_DUP_ERROR -3
#define INVALID_KEY_DATA -4
#define EMPTY_DBT_DATA -5
#define ITERATOR_AT_END -6
#define CURSOR_NOT_OPEN -7
#define BDBOP(bdb_call, ret) do { 					\
	if ((ret = (bdb_call)) != 0) throw_bdb_exception(#bdb_call, ret);\
	} while(0)
#define BDBOP2(bdb_call, ret, cleanup) do {				\
	if ((ret = (bdb_call)) != 0) { (cleanup); 			\
		throw_bdb_exception(#bdb_call, ret);}			\
	} while (0)
#define BDBOP3(bdb_call, ret, exception, cleanup) do {			\
	if (((ret = (bdb_call)) != 0) && (ret & exception) == 0) {	\
		(cleanup); throw_bdb_exception(#bdb_call, ret);}	\
	} while (0)
#define THROW(exception_type, arg_list) do {		\
	exception_type ex arg_list; throw ex; } while (0)
#define THROW0(exception_type)	do {			\
	exception_type ex; throw ex; } while (0)
#define INVALID_INDEX ((index_type)-1)
#define INVALID_DLEN ((u_int32_t)-1)
#define DBSTL_MAX_DATA_BUF_LEN 1024 * 4096
#define DBSTL_MAX_KEY_BUF_LEN 1024 * 4096
#define DBSTL_MAX_MTX_ENV_MUTEX 4096 * 4
#define DBSTL_BULK_BUF_SIZE 256 * 1024
#define COMPARE_CHECK(obj) if (this == &obj) return true;
#define ASSIGNMENT_PREDCOND(obj) if (this == &obj) return obj;
START_NS(dbstl)
_exported void close_db(Db *pdb);
_exported void close_all_dbs();
_exported void close_db_env(DbEnv *pdbenv);
_exported void close_all_db_envs();
_exported DbTxn* begin_txn(u_int32_t flags, DbEnv *env);
_exported void commit_txn(DbEnv *env, u_int32_t flags = 0);
_exported void commit_txn(DbEnv *env, DbTxn *txn, u_int32_t flags = 0);
_exported void abort_txn(DbEnv *env);
_exported void abort_txn(DbEnv *env, DbTxn *txn);
_exported DbTxn* current_txn(DbEnv *env);
_exported DbTxn* set_current_txn_handle(DbEnv *env, DbTxn *newtxn);
_exported void register_db(Db *pdb1);
_exported void register_db_env(DbEnv *env1);
_exported Db* open_db (DbEnv *penv, const char *filename, DBTYPE dbtype,
    u_int32_t oflags, u_int32_t set_flags, int mode = 0644, DbTxn *txn = NULL,
    u_int32_t cflags = 0, const char* dbname = NULL);
_exported DbEnv* open_env(const char *env_home, u_int32_t set_flags,
    u_int32_t oflags = DB_CREATE | DB_INIT_MPOOL,
    u_int32_t cachesize = 4 * 1024 * 1024,
    int mode = 0644,
    u_int32_t cflags = 0 );
_exported db_mutex_t alloc_mutex();
_exported int lock_mutex(db_mutex_t mtx);
_exported int unlock_mutex(db_mutex_t mtx);
_exported void free_mutex(db_mutex_t mtx);
_exported size_t close_db_cursors(Db* dbp1);
_exported void dbstl_startup();
_exported void dbstl_exit();
_exported bool operator==(const Dbt&d1, const Dbt&d2);
_exported bool operator==(const DBT&d1, const DBT&d2);
_exported void set_global_dbfile_suffix_number(u_int32_t num);
_exported void * DbstlReAlloc(void *ptr, size_t size);
_exported void * DbstlMalloc(size_t size);
_exported u_int32_t hash_default(Db *  , const void *key, u_int32_t len);
_exported u_int32_t dbstl_strlen(const char *str);
_exported void dbstl_strcpy(char *dest, const char *src, size_t num);
_exported int dbstl_strncmp(const char *s1, const char *s2, size_t num);
_exported int dbstl_strcmp(const char *s1, const char *s2);
_exported int dbstl_wcscmp(const wchar_t *s1, const wchar_t *s2);
_exported int dbstl_wcsncmp(const wchar_t *s1, const wchar_t *s2, size_t num);
_exported u_int32_t dbstl_wcslen(const wchar_t *str);
_exported void dbstl_wcscpy(wchar_t *dest, const wchar_t *src, size_t num);
END_NS
#endif
