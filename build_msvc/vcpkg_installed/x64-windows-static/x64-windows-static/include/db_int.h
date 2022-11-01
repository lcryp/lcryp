#ifndef _DB_INT_H_
#define	_DB_INT_H_
#ifdef HAVE_SYSTEM_INCLUDE_FILES
#include <sys/types.h>
#ifdef DIAG_MVCC
#include <sys/mman.h>
#endif
#include <sys/stat.h>
#if defined(__INCLUDE_SELECT_H)
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif
#ifdef HAVE_VXWORKS
#include <selectLib.h>
#endif
#endif
#if TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#else
#if HAVE_SYS_TIME_H
#include <sys/time.h>
#else
#include <time.h>
#endif
#endif
#ifdef HAVE_VXWORKS
#include <net/uio.h>
#else
#include <sys/uio.h>
#endif
#if defined(__INCLUDE_NETWORKING)
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#endif
#if defined(STDC_HEADERS) || defined(__cplusplus)
#include <stdarg.h>
#else
#include <varargs.h>
#endif
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#if defined(__INCLUDE_DIRECTORY)
#if HAVE_DIRENT_H
# include <dirent.h>
# define NAMLEN(dirent) strlen((dirent)->d_name)
#else
# define dirent direct
# define NAMLEN(dirent) (dirent)->d_namlen
# if HAVE_SYS_NDIR_H
#  include <sys/ndir.h>
# endif
# if HAVE_SYS_DIR_H
#  include <sys/dir.h>
# endif
# if HAVE_NDIR_H
#  include <ndir.h>
# endif
#endif
#endif
#endif
#ifdef DB_WIN32
#include "dbinc/win_db.h"
#endif
#include "db.h"
#include "clib_port.h"
#include "dbinc/queue.h"
#include "dbinc/shqueue.h"
#if defined(__cplusplus)
extern "C" {
#endif
struct __db_reginfo_t;	typedef struct __db_reginfo_t REGINFO;
struct __db_txnhead;	typedef struct __db_txnhead DB_TXNHEAD;
struct __db_txnlist;	typedef struct __db_txnlist DB_TXNLIST;
struct __vrfy_childinfo;typedef struct __vrfy_childinfo VRFY_CHILDINFO;
struct __vrfy_dbinfo;   typedef struct __vrfy_dbinfo VRFY_DBINFO;
struct __vrfy_pageinfo; typedef struct __vrfy_pageinfo VRFY_PAGEINFO;
typedef SH_TAILQ_HEAD(__hash_head) DB_HASHTAB;
#undef	FALSE
#define	FALSE		0
#undef	TRUE
#define	TRUE		(!FALSE)
#define	MEGABYTE	1048576
#define	GIGABYTE	1073741824
#define	NS_PER_MS	1000000
#define	NS_PER_US	1000
#define	NS_PER_SEC	1000000000
#define	US_PER_MS	1000
#define	US_PER_SEC	1000000
#define	MS_PER_SEC	1000
#define	RECNO_OOB	0
#define	POWER_OF_TWO(x)	(((x) & ((x) - 1)) == 0)
#define	DB_MIN_PGSIZE	0x000200
#define	DB_MAX_PGSIZE	0x010000
#define	IS_VALID_PAGESIZE(x)						\
	(POWER_OF_TWO(x) && (x) >= DB_MIN_PGSIZE && ((x) <= DB_MAX_PGSIZE))
#define	DB_MINPAGECACHE	16
#define	DB_DEF_IOSIZE	(8 * 1024)
#undef	DB_ALIGN
#define	DB_ALIGN(v, bound)						\
	(((v) + (bound) - 1) & ~(((uintmax_t)(bound)) - 1))
#undef	ALIGNP_INC
#define	ALIGNP_INC(p, bound)						\
	(void *)(((uintptr_t)(p) + (bound) - 1) & ~(((uintptr_t)(bound)) - 1))
#define	P_TO_ULONG(p)	((u_long)(uintptr_t)(p))
#define	P_TO_UINT32(p)	((u_int32_t)(uintptr_t)(p))
#define	P_TO_UINT16(p)	((u_int16_t)(uintptr_t)(p))
#undef	SSZ
#define	SSZ(name, field)  P_TO_UINT16(&(((name *)0)->field))
#undef	SSZA
#define	SSZA(name, field) P_TO_UINT16(&(((name *)0)->field[0]))
typedef struct __fn {
	u_int32_t mask;
	const char *name;
} FN;
#define	FLD_CLR(fld, f)		(fld) &= ~(f)
#define	FLD_ISSET(fld, f)	((fld) & (f))
#define	FLD_SET(fld, f)		(fld) |= (f)
#define	F_CLR(p, f)		(p)->flags &= ~(f)
#define	F_ISSET(p, f)		((p)->flags & (f))
#define	F_SET(p, f)		(p)->flags |= (f)
#define	LF_CLR(f)		((flags) &= ~(f))
#define	LF_ISSET(f)		((flags) & (f))
#define	LF_SET(f)		((flags) |= (f))
#define	DB_PCT(v, total)						\
	((int)((total) == 0 ? 0 : ((double)(v) * 100) / (total)))
#define	DB_PCT_PG(v, total, pgsize)					\
	((int)((total) == 0 ? 0 :					\
	    100 - ((double)(v) * 100) / (((double)total) * (pgsize))))
#undef	STAT
#ifdef	HAVE_STATISTICS
#define	STAT(x)	x
#else
#define	STAT(x)
#endif
typedef struct __db_msgbuf {
	char *buf;
	char *cur;
	size_t len;
} DB_MSGBUF;
#define	DB_MSGBUF_INIT(a) do {						\
	(a)->buf = (a)->cur = NULL;					\
	(a)->len = 0;							\
} while (0)
#define	DB_MSGBUF_FLUSH(env, a) do {					\
	if ((a)->buf != NULL) {						\
		if ((a)->cur != (a)->buf)				\
			__db_msg(env, "%s", (a)->buf);		\
		__os_free(env, (a)->buf);				\
		DB_MSGBUF_INIT(a);					\
	}								\
} while (0)
#define	STAT_FMT(msg, fmt, type, v) do {				\
	DB_MSGBUF __mb;							\
	DB_MSGBUF_INIT(&__mb);						\
	__db_msgadd(env, &__mb, fmt, (type)(v));			\
	__db_msgadd(env, &__mb, "\t%s", msg);				\
	DB_MSGBUF_FLUSH(env, &__mb);					\
} while (0)
#define	STAT_HEX(msg, v)						\
	__db_msg(env, "%#lx\t%s", (u_long)(v), msg)
#define	STAT_ISSET(msg, p)						\
	__db_msg(env, "%sSet\t%s", (p) == NULL ? "!" : " ", msg)
#define	STAT_LONG(msg, v)						\
	__db_msg(env, "%ld\t%s", (long)(v), msg)
#define	STAT_LSN(msg, lsnp)						\
	__db_msg(env, "%lu/%lu\t%s",					\
	    (u_long)(lsnp)->file, (u_long)(lsnp)->offset, msg)
#define	STAT_POINTER(msg, v)						\
	__db_msg(env, "%#lx\t%s", P_TO_ULONG(v), msg)
#define	STAT_STRING(msg, p) do {					\
	const char *__p = p;	 		\
	__db_msg(env, "%s\t%s", __p == NULL ? "!Set" : __p, msg);	\
} while (0)
#define	STAT_ULONG(msg, v)						\
	__db_msg(env, "%lu\t%s", (u_long)(v), msg)
#define	DB_SET_DBT(dbt, d, s)  do {					\
	(dbt).data = (void *)(d);					\
	(dbt).size = (u_int32_t)(s);					\
} while (0)
#define	DB_INIT_DBT(dbt, d, s)  do {					\
	memset(&(dbt), 0, sizeof(dbt));					\
	DB_SET_DBT(dbt, d, s);						\
} while (0)
#define	DB_RETOK_STD(ret)	((ret) == 0)
#define	DB_RETOK_DBCDEL(ret)	((ret) == 0 || (ret) == DB_KEYEMPTY || \
				    (ret) == DB_NOTFOUND)
#define	DB_RETOK_DBCGET(ret)	((ret) == 0 || (ret) == DB_KEYEMPTY || \
				    (ret) == DB_NOTFOUND)
#define	DB_RETOK_DBCPUT(ret)	((ret) == 0 || (ret) == DB_KEYEXIST || \
				    (ret) == DB_NOTFOUND)
#define	DB_RETOK_DBDEL(ret)	DB_RETOK_DBCDEL(ret)
#define	DB_RETOK_DBGET(ret)	DB_RETOK_DBCGET(ret)
#define	DB_RETOK_DBPUT(ret)	((ret) == 0 || (ret) == DB_KEYEXIST)
#define	DB_RETOK_EXISTS(ret)	DB_RETOK_DBCGET(ret)
#define	DB_RETOK_LGGET(ret)	((ret) == 0 || (ret) == DB_NOTFOUND)
#define	DB_RETOK_MPGET(ret)	((ret) == 0 || (ret) == DB_PAGE_NOTFOUND)
#define	DB_RETOK_REPPMSG(ret)	((ret) == 0 || \
				    (ret) == DB_REP_IGNORE || \
				    (ret) == DB_REP_ISPERM || \
				    (ret) == DB_REP_NEWMASTER || \
				    (ret) == DB_REP_NEWSITE || \
				    (ret) == DB_REP_NOTPERM)
#define	DB_RETOK_REPMGR_START(ret) ((ret) == 0 || (ret) == DB_REP_IGNORE)
#ifdef	EOPNOTSUPP
#define	DB_OPNOTSUP	EOPNOTSUPP
#else
#ifdef	ENOTSUP
#define	DB_OPNOTSUP	ENOTSUP
#else
#define	DB_OPNOTSUP	EINVAL
#endif
#endif
#define	DB_MAXPATHLEN	1024
#define	PATH_DOT	"."
#define	PATH_SEPARATOR	"\\/:"
typedef enum {
	DB_APP_NONE=0,
	DB_APP_DATA,
	DB_APP_LOG,
	DB_APP_TMP,
	DB_APP_RECOVER
} APPNAME;
#define	ALIVE_ON(env)		((env)->dbenv->is_alive != NULL)
#define	CDB_LOCKING(env)	F_ISSET(env, ENV_CDB)
#define	CRYPTO_ON(env)		((env)->crypto_handle != NULL)
#define	LOCKING_ON(env)		((env)->lk_handle != NULL)
#define	LOGGING_ON(env)		((env)->lg_handle != NULL)
#define	MPOOL_ON(env)		((env)->mp_handle != NULL)
#define	MUTEX_ON(env)		((env)->mutex_handle != NULL)
#define	REP_ON(env)							\
	((env)->rep_handle != NULL && (env)->rep_handle->region != NULL)
#define	RPC_ON(dbenv)		((dbenv)->cl_handle != NULL)
#define	TXN_ON(env)		((env)->tx_handle != NULL)
#define	STD_LOCKING(dbc)						\
	(!F_ISSET(dbc, DBC_OPD) &&					\
	    !CDB_LOCKING((dbc)->env) && LOCKING_ON((dbc)->env))
#define	IS_RECOVERING(env)						\
	(LOGGING_ON(env) && F_ISSET((env)->lg_handle, DBLOG_RECOVER))
#define	ENV_ILLEGAL_AFTER_OPEN(env, name)				\
	if (F_ISSET((env), ENV_OPEN_CALLED))				\
		return (__db_mi_open(env, name, 1));
#define	ENV_ILLEGAL_BEFORE_OPEN(env, name)				\
	if (!F_ISSET((env), ENV_OPEN_CALLED))				\
		return (__db_mi_open(env, name, 0));
#define	ENV_REQUIRES_CONFIG(env, handle, i, flags)			\
	if (handle == NULL)						\
		return (__env_not_config(env, i, flags));
#define	ENV_REQUIRES_CONFIG_XX(env, handle, i, flags)			\
	if ((env)->handle->region == NULL)				\
		return (__env_not_config(env, i, flags));
#define	ENV_NOT_CONFIGURED(env, handle, i, flags)			\
	if (F_ISSET((env), ENV_OPEN_CALLED))				\
		ENV_REQUIRES_CONFIG(env, handle, i, flags)
#define	ENV_ENTER(env, ip) do {						\
	int __ret;							\
	PANIC_CHECK(env);						\
	if ((env)->thr_hashtab == NULL)					\
		ip = NULL;						\
	else {								\
		if ((__ret =						\
		    __env_set_state(env, &(ip), THREAD_ACTIVE)) != 0)	\
			return (__ret);					\
	}								\
} while (0)
#define	FAILCHK_THREAD(env, ip) do {					\
	if ((ip) != NULL)						\
		(ip)->dbth_state = THREAD_FAILCHK;			\
} while (0)
#define	ENV_GET_THREAD_INFO(env, ip) ENV_ENTER(env, ip)
#ifdef DIAGNOSTIC
#define	ENV_LEAVE(env, ip) do {						\
	if ((ip) != NULL) {						\
		DB_ASSERT(env, ((ip)->dbth_state == THREAD_ACTIVE  ||	\
		    (ip)->dbth_state == THREAD_FAILCHK));		\
		(ip)->dbth_state = THREAD_OUT;				\
	}								\
} while (0)
#else
#define	ENV_LEAVE(env, ip) do {						\
	if ((ip) != NULL)						\
		(ip)->dbth_state = THREAD_OUT;				\
} while (0)
#endif
#ifdef DIAGNOSTIC
#define	CHECK_THREAD(env) do {						\
	if ((env)->thr_hashtab != NULL)					\
		(void)__env_set_state(env, NULL, THREAD_VERIFY);	\
} while (0)
#ifdef HAVE_STATISTICS
#define	CHECK_MTX_THREAD(env, mtx) do {					\
	if (mtx->alloc_id != MTX_MUTEX_REGION &&			\
	    mtx->alloc_id != MTX_ENV_REGION &&				\
	    mtx->alloc_id != MTX_APPLICATION)				\
		CHECK_THREAD(env);					\
} while (0)
#else
#define	CHECK_MTX_THREAD(env, mtx)
#endif
#else
#define	CHECK_THREAD(env)
#define	CHECK_MTX_THREAD(env, mtx)
#endif
typedef enum {
	THREAD_SLOT_NOT_IN_USE=0,
	THREAD_OUT,
	THREAD_ACTIVE,
	THREAD_BLOCKED,
	THREAD_BLOCKED_DEAD,
	THREAD_FAILCHK,
	THREAD_VERIFY
} DB_THREAD_STATE;
typedef struct __pin_list {
	roff_t b_ref;
	int region;
} PIN_LIST;
#define	PINMAX 4
struct __db_thread_info {
	pid_t		dbth_pid;
	db_threadid_t	dbth_tid;
	DB_THREAD_STATE	dbth_state;
	SH_TAILQ_ENTRY	dbth_links;
	u_int16_t	dbth_pincount;
	u_int16_t	dbth_pinmax;
	roff_t		dbth_pinlist;
	PIN_LIST	dbth_pinarray[PINMAX];
};
typedef struct __env_thread_info {
	u_int32_t	thr_count;
	u_int32_t	thr_max;
	u_int32_t	thr_nbucket;
	roff_t		thr_hashoff;
} THREAD_INFO;
#define	DB_EVENT(env, e, einfo) do {					\
	DB_ENV *__dbenv = (env)->dbenv;					\
	if (__dbenv->db_event_func != NULL)				\
		__dbenv->db_event_func(__dbenv, e, einfo);		\
} while (0)
typedef struct __flag_map {
	u_int32_t inflag, outflag;
} FLAG_MAP;
struct __env {
	DB_ENV *dbenv;
	db_mutex_t mtx_env;
	char	 *db_home;
	u_int32_t open_flags;
	int	  db_mode;
	pid_t	pid_cache;
	DB_FH	*lockfhp;
	DB_LOCKER *env_lref;
	DB_DISTAB   recover_dtab;
	int dir_mode;
	u_int32_t	 thr_nbucket;
	DB_HASHTAB	*thr_hashtab;
	struct {
		int	  alloc_id;
		u_int32_t flags;
	} *mutex_iq;
	u_int		mutex_iq_next;
	u_int		mutex_iq_max;
	db_mutex_t mtx_dblist;
	int	   db_ref;
	TAILQ_HEAD(__dblist, __db) dblist;
	TAILQ_HEAD(__fdlist, __fh_t) fdlist;
	db_mutex_t	 mtx_mt;
	int		 mti;
	u_long		*mt;
	DB_CIPHER	*crypto_handle;
	DB_LOCKTAB	*lk_handle;
	DB_LOG		*lg_handle;
	DB_MPOOL	*mp_handle;
	DB_MUTEXMGR	*mutex_handle;
	DB_REP		*rep_handle;
	DB_TXNMGR	*tx_handle;
#define	DB_USERCOPY_GETDATA	0x0001
#define	DB_USERCOPY_SETDATA	0x0002
	int (*dbt_usercopy)
	    __P((DBT *, u_int32_t, void *, u_int32_t, u_int32_t));
	REGINFO	*reginfo;
#define	DB_TEST_ELECTINIT	 1
#define	DB_TEST_ELECTVOTE1	 2
#define	DB_TEST_POSTDESTROY	 3
#define	DB_TEST_POSTLOG		 4
#define	DB_TEST_POSTLOGMETA	 5
#define	DB_TEST_POSTOPEN	 6
#define	DB_TEST_POSTSYNC	 7
#define	DB_TEST_PREDESTROY	 8
#define	DB_TEST_PREOPEN		 9
#define	DB_TEST_SUBDB_LOCKS	 10
	int	test_abort;
	int	test_check;
	int	test_copy;
#define	ENV_CDB			0x00000001
#define	ENV_DBLOCAL		0x00000002
#define	ENV_LITTLEENDIAN	0x00000004
#define	ENV_LOCKDOWN		0x00000008
#define	ENV_NO_OUTPUT_SET	0x00000010
#define	ENV_OPEN_CALLED		0x00000020
#define	ENV_PRIVATE		0x00000040
#define	ENV_RECOVER_FATAL	0x00000080
#define	ENV_REF_COUNTED		0x00000100
#define	ENV_SYSTEM_MEM		0x00000200
#define	ENV_THREAD		0x00000400
	u_int32_t flags;
};
#define	DB_IS_THREADED(dbp)						\
	((dbp)->mutex != MUTEX_INVALID)
#define	DB_ILLEGAL_AFTER_OPEN(dbp, name)				\
	if (F_ISSET((dbp), DB_AM_OPEN_CALLED))				\
		return (__db_mi_open((dbp)->env, name, 1));
#define	DB_ILLEGAL_BEFORE_OPEN(dbp, name)				\
	if (!F_ISSET((dbp), DB_AM_OPEN_CALLED))				\
		return (__db_mi_open((dbp)->env, name, 0));
#define	DB_ILLEGAL_IN_ENV(dbp, name)					\
	if (!F_ISSET((dbp)->env, ENV_DBLOCAL))				\
		return (__db_mi_env((dbp)->env, name));
#define	DB_ILLEGAL_METHOD(dbp, flags) {					\
	int __ret;							\
	if ((__ret = __dbh_am_chk(dbp, flags)) != 0)			\
		return (__ret);						\
}
#define	__DBC_INTERNAL							\
	DBC	 *opd;			 \
	DBC	 *pdbc;			  \
									\
	void	 *page;			 		\
	u_int32_t part;			 		\
	db_pgno_t root;			 		\
	db_pgno_t pgno;			 	\
	db_indx_t indx;			 \
									\
	 				\
	db_pgno_t stream_start_pgno;	 		\
	u_int32_t stream_off;		 		\
	db_pgno_t stream_curr_pgno;	 	\
									\
	DB_LOCK		lock;		 		\
	db_lockmode_t	lock_mode;
struct __dbc_internal {
	__DBC_INTERNAL
};
typedef enum { MU_REMOVE, MU_RENAME, MU_OPEN } mu_action;
#ifdef HAVE_PARTITION
#define	IS_INITIALIZED(dbc)	(DB_IS_PARTITIONED((dbc)->dbp) ?	\
		((PART_CURSOR *)(dbc)->internal)->sub_cursor != NULL && \
		((PART_CURSOR *)(dbc)->internal)->sub_cursor->		\
		    internal->pgno != PGNO_INVALID :			\
		(dbc)->internal->pgno != PGNO_INVALID)
#else
#define	IS_INITIALIZED(dbc)	((dbc)->internal->pgno != PGNO_INVALID)
#endif
#define	FREE_IF_NEEDED(env, dbt)					\
	if (F_ISSET((dbt), DB_DBT_APPMALLOC)) {				\
		__os_ufree((env), (dbt)->data);				\
		F_CLR((dbt), DB_DBT_APPMALLOC);				\
	}
#define	SET_RET_MEM(dbc, owner)				\
	do {						\
		(dbc)->rskey = &(owner)->my_rskey;	\
		(dbc)->rkey = &(owner)->my_rkey;	\
		(dbc)->rdata = &(owner)->my_rdata;	\
	} while (0)
#define	COPY_RET_MEM(src, dest)				\
	do {						\
		(dest)->rskey = (src)->rskey;		\
		(dest)->rkey = (src)->rkey;		\
		(dest)->rdata = (src)->rdata;		\
	} while (0)
#define	RESET_RET_MEM(dbc)				\
	do {						\
		(dbc)->rskey = &(dbc)->my_rskey;	\
		(dbc)->rkey = &(dbc)->my_rkey;		\
		(dbc)->rdata = &(dbc)->my_rdata;	\
	} while (0)
#define	DB_FTYPE_SET		-1
#define	DB_FTYPE_NOTSET		 0
#define	DB_LSN_OFF_NOTSET	-1
#define	DB_CLEARLEN_NOTSET	UINT32_MAX
typedef struct __dbpginfo {
	size_t	db_pagesize;
	u_int32_t flags;
	DBTYPE  type;
} DB_PGINFO;
#define	ZERO_LSN(LSN) do {						\
	(LSN).file = 0;							\
	(LSN).offset = 0;						\
} while (0)
#define	IS_ZERO_LSN(LSN)	((LSN).file == 0 && (LSN).offset == 0)
#define	IS_INIT_LSN(LSN)	((LSN).file == 1 && (LSN).offset == 0)
#define	INIT_LSN(LSN)		do {					\
	(LSN).file = 1;							\
	(LSN).offset = 0;						\
} while (0)
#define	MAX_LSN(LSN) do {						\
	(LSN).file = UINT32_MAX;					\
	(LSN).offset = UINT32_MAX;					\
} while (0)
#define	IS_MAX_LSN(LSN) \
	((LSN).file == UINT32_MAX && (LSN).offset == UINT32_MAX)
#define	LSN_NOT_LOGGED(LSN) do {					\
	(LSN).file = 0;							\
	(LSN).offset = 1;						\
} while (0)
#define	IS_NOT_LOGGED_LSN(LSN) \
	((LSN).file == 0 && (LSN).offset == 1)
#define	LOG_COMPARE(lsn0, lsn1)						\
    ((lsn0)->file != (lsn1)->file ?					\
    ((lsn0)->file < (lsn1)->file ? -1 : 1) :				\
    ((lsn0)->offset != (lsn1)->offset ?					\
    ((lsn0)->offset < (lsn1)->offset ? -1 : 1) : 0))
#define	DB_NONBLOCK(C)	((C)->txn != NULL && F_ISSET((C)->txn, TXN_NOWAIT))
#define	NOWAIT_FLAG(txn) \
	((txn) != NULL && F_ISSET((txn), TXN_NOWAIT) ? DB_LOCK_NOWAIT : 0)
#define	IS_REAL_TXN(txn)						\
	((txn) != NULL && !F_ISSET(txn, TXN_CDSGROUP))
#define	IS_SUBTRANSACTION(txn)						\
	((txn) != NULL && (txn)->parent != NULL)
#define	DB_IV_BYTES     16
#define	DB_MAC_KEY	20
#define CMP_INT_SPARE_VAL	0xFC
#ifdef CONFIG_TEST
#define	DB_RPC2ND_MASK		0x00f00000
#define	DB_RPC2ND_REVERSEDATA	0x00100000
#define	DB_RPC2ND_NOOP		0x00200000
#define	DB_RPC2ND_CONCATKEYDATA	0x00300000
#define	DB_RPC2ND_CONCATDATAKEY 0x00400000
#define	DB_RPC2ND_REVERSECONCAT	0x00500000
#define	DB_RPC2ND_TRUNCDATA	0x00600000
#define	DB_RPC2ND_CONSTANT	0x00700000
#define	DB_RPC2ND_GETZIP	0x00800000
#define	DB_RPC2ND_GETNAME	0x00900000
#endif
#if defined(__cplusplus)
}
#endif
#include "dbinc/globals.h"
#include "dbinc/clock.h"
#include "dbinc/debug.h"
#include "dbinc/region.h"
#include "dbinc_auto/env_ext.h"
#include "dbinc/mutex.h"
#ifdef HAVE_REPLICATION_THREADS
#include "dbinc/repmgr.h"
#endif
#include "dbinc/rep.h"
#include "dbinc/os.h"
#include "dbinc_auto/clib_ext.h"
#include "dbinc_auto/common_ext.h"
#define	DBENV_LOGGING(env)						\
	(LOGGING_ON(env) && !IS_REP_CLIENT(env) && (!IS_RECOVERING(env)))
#if defined(DIAGNOSTIC) || defined(DEBUG_ROP)  || defined(DEBUG_WOP)
#define	DBC_LOGGING(dbc)	__dbc_logging(dbc)
#else
#define	DBC_LOGGING(dbc)						\
	((dbc)->txn != NULL && LOGGING_ON((dbc)->env) &&		\
	    !F_ISSET((dbc), DBC_RECOVER) && !IS_REP_CLIENT((dbc)->env))
#endif
#endif
