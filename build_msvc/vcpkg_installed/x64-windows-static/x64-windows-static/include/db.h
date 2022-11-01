#ifndef _DB_H_
#define	_DB_H_
#ifndef	__NO_SYSTEM_INCLUDES
#include <sys/types.h>
#include <stddef.h>
#include <stdio.h>
#endif
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4820)
#endif
#if defined(__cplusplus)
extern "C" {
#endif
#undef __P
#define	__P(protos)	protos
#define	DB_VERSION_MAJOR	4
#define	DB_VERSION_MINOR	8
#define	DB_VERSION_PATCH	30
#define	DB_VERSION_STRING	"Berkeley DB 4.8.30: (April 12, 2010)"
#ifndef	__BIT_TYPES_DEFINED__
#define	__BIT_TYPES_DEFINED__
typedef unsigned char u_int8_t;
typedef short int16_t;
typedef unsigned short u_int16_t;
typedef int int32_t;
typedef unsigned int u_int32_t;
typedef __int64 int64_t;
typedef unsigned __int64 u_int64_t;
#endif
#ifndef _WINSOCKAPI_
typedef unsigned char u_char;
typedef unsigned int u_int;
typedef unsigned long u_long;
#endif
typedef unsigned short u_short;
#if defined(_MSC_VER) && _MSC_VER < 1300
typedef u_int32_t uintmax_t;
#else
typedef u_int64_t uintmax_t;
#endif
#ifdef _WIN64
typedef u_int64_t uintptr_t;
#else
typedef u_int32_t uintptr_t;
#endif
#define	off_t	__db_off_t
typedef int64_t off_t;
typedef int pid_t;
#ifdef _WIN64
typedef int64_t ssize_t;
#else
typedef int32_t ssize_t;
#endif
typedef int64_t db_seq_t;
typedef u_int32_t db_threadid_t;
typedef	u_int32_t	db_pgno_t;
typedef	u_int16_t	db_indx_t;
#define	DB_MAX_PAGES	0xffffffff
typedef	u_int32_t	db_recno_t;
#define	DB_MAX_RECORDS	0xffffffff
typedef u_int32_t	db_timeout_t;
typedef	uintptr_t	roff_t;
struct __db;		typedef struct __db DB;
struct __db_bt_stat;	typedef struct __db_bt_stat DB_BTREE_STAT;
struct __db_cipher;	typedef struct __db_cipher DB_CIPHER;
struct __db_compact;	typedef struct __db_compact DB_COMPACT;
struct __db_dbt;	typedef struct __db_dbt DBT;
struct __db_distab;	typedef struct __db_distab DB_DISTAB;
struct __db_env;	typedef struct __db_env DB_ENV;
struct __db_h_stat;	typedef struct __db_h_stat DB_HASH_STAT;
struct __db_ilock;	typedef struct __db_ilock DB_LOCK_ILOCK;
struct __db_lock_hstat;	typedef struct __db_lock_hstat DB_LOCK_HSTAT;
struct __db_lock_pstat;	typedef struct __db_lock_pstat DB_LOCK_PSTAT;
struct __db_lock_stat;	typedef struct __db_lock_stat DB_LOCK_STAT;
struct __db_lock_u;	typedef struct __db_lock_u DB_LOCK;
struct __db_locker;	typedef struct __db_locker DB_LOCKER;
struct __db_lockreq;	typedef struct __db_lockreq DB_LOCKREQ;
struct __db_locktab;	typedef struct __db_locktab DB_LOCKTAB;
struct __db_log;	typedef struct __db_log DB_LOG;
struct __db_log_cursor;	typedef struct __db_log_cursor DB_LOGC;
struct __db_log_stat;	typedef struct __db_log_stat DB_LOG_STAT;
struct __db_lsn;	typedef struct __db_lsn DB_LSN;
struct __db_mpool;	typedef struct __db_mpool DB_MPOOL;
struct __db_mpool_fstat;typedef struct __db_mpool_fstat DB_MPOOL_FSTAT;
struct __db_mpool_stat;	typedef struct __db_mpool_stat DB_MPOOL_STAT;
struct __db_mpoolfile;	typedef struct __db_mpoolfile DB_MPOOLFILE;
struct __db_mutex_stat;	typedef struct __db_mutex_stat DB_MUTEX_STAT;
struct __db_mutex_t;	typedef struct __db_mutex_t DB_MUTEX;
struct __db_mutexmgr;	typedef struct __db_mutexmgr DB_MUTEXMGR;
struct __db_preplist;	typedef struct __db_preplist DB_PREPLIST;
struct __db_qam_stat;	typedef struct __db_qam_stat DB_QUEUE_STAT;
struct __db_rep;	typedef struct __db_rep DB_REP;
struct __db_rep_stat;	typedef struct __db_rep_stat DB_REP_STAT;
struct __db_repmgr_site;typedef struct __db_repmgr_site DB_REPMGR_SITE;
struct __db_repmgr_stat;typedef struct __db_repmgr_stat DB_REPMGR_STAT;
struct __db_seq_record; typedef struct __db_seq_record DB_SEQ_RECORD;
struct __db_seq_stat;	typedef struct __db_seq_stat DB_SEQUENCE_STAT;
struct __db_sequence;	typedef struct __db_sequence DB_SEQUENCE;
struct __db_thread_info;typedef struct __db_thread_info DB_THREAD_INFO;
struct __db_txn;	typedef struct __db_txn DB_TXN;
struct __db_txn_active;	typedef struct __db_txn_active DB_TXN_ACTIVE;
struct __db_txn_stat;	typedef struct __db_txn_stat DB_TXN_STAT;
struct __db_txnmgr;	typedef struct __db_txnmgr DB_TXNMGR;
struct __dbc;		typedef struct __dbc DBC;
struct __dbc_internal;	typedef struct __dbc_internal DBC_INTERNAL;
struct __env;		typedef struct __env ENV;
struct __fh_t;		typedef struct __fh_t DB_FH;
struct __fname;		typedef struct __fname FNAME;
struct __key_range;	typedef struct __key_range DB_KEY_RANGE;
struct __mpoolfile;	typedef struct __mpoolfile MPOOLFILE;
#define	DB_DEGREE_2	      DB_READ_COMMITTED
#define	DB_DIRTY_READ	      DB_READ_UNCOMMITTED
#define	DB_JOINENV	      0x0
struct __db_dbt {
	void	 *data;
	u_int32_t size;
	u_int32_t ulen;
	u_int32_t dlen;
	u_int32_t doff;
	void *app_data;
#define	DB_DBT_APPMALLOC	0x001
#define	DB_DBT_BULK		0x002
#define	DB_DBT_DUPOK		0x004
#define	DB_DBT_ISSET		0x008
#define	DB_DBT_MALLOC		0x010
#define	DB_DBT_MULTIPLE		0x020
#define	DB_DBT_PARTIAL		0x040
#define	DB_DBT_REALLOC		0x080
#define	DB_DBT_STREAMING	0x100
#define	DB_DBT_USERCOPY		0x200
#define	DB_DBT_USERMEM		0x400
	u_int32_t flags;
};
typedef u_int32_t	db_mutex_t;
struct __db_mutex_stat {
	u_int32_t st_mutex_align;
	u_int32_t st_mutex_tas_spins;
	u_int32_t st_mutex_cnt;
	u_int32_t st_mutex_free;
	u_int32_t st_mutex_inuse;
	u_int32_t st_mutex_inuse_max;
#ifndef __TEST_DB_NO_STATISTICS
	uintmax_t st_region_wait;
	uintmax_t st_region_nowait;
	roff_t	  st_regsize;
#endif
};
#define	DB_THREADID_STRLEN	128
#define	DB_LOCKVERSION	1
#define	DB_FILE_ID_LEN		20
#define	DB_LOCK_NORUN		0
#define	DB_LOCK_DEFAULT		1
#define	DB_LOCK_EXPIRE		2
#define	DB_LOCK_MAXLOCKS	3
#define	DB_LOCK_MAXWRITE	4
#define	DB_LOCK_MINLOCKS	5
#define	DB_LOCK_MINWRITE	6
#define	DB_LOCK_OLDEST		7
#define	DB_LOCK_RANDOM		8
#define	DB_LOCK_YOUNGEST	9
typedef enum {
	DB_LOCK_NG=0,
	DB_LOCK_READ=1,
	DB_LOCK_WRITE=2,
	DB_LOCK_WAIT=3,
	DB_LOCK_IWRITE=4,
	DB_LOCK_IREAD=5,
	DB_LOCK_IWR=6,
	DB_LOCK_READ_UNCOMMITTED=7,
	DB_LOCK_WWRITE=8
} db_lockmode_t;
typedef enum {
	DB_LOCK_DUMP=0,
	DB_LOCK_GET=1,
	DB_LOCK_GET_TIMEOUT=2,
	DB_LOCK_INHERIT=3,
	DB_LOCK_PUT=4,
	DB_LOCK_PUT_ALL=5,
	DB_LOCK_PUT_OBJ=6,
	DB_LOCK_PUT_READ=7,
	DB_LOCK_TIMEOUT=8,
	DB_LOCK_TRADE=9,
	DB_LOCK_UPGRADE_WRITE=10
} db_lockop_t;
typedef enum  {
	DB_LSTAT_ABORTED=1,
	DB_LSTAT_EXPIRED=2,
	DB_LSTAT_FREE=3,
	DB_LSTAT_HELD=4,
	DB_LSTAT_PENDING=5,
	DB_LSTAT_WAITING=6
}db_status_t;
struct __db_lock_stat {
	u_int32_t st_id;
	u_int32_t st_cur_maxid;
	u_int32_t st_maxlocks;
	u_int32_t st_maxlockers;
	u_int32_t st_maxobjects;
	u_int32_t st_partitions;
	int	  st_nmodes;
	u_int32_t st_nlockers;
#ifndef __TEST_DB_NO_STATISTICS
	u_int32_t st_nlocks;
	u_int32_t st_maxnlocks;
	u_int32_t st_maxhlocks;
	uintmax_t st_locksteals;
	uintmax_t st_maxlsteals;
	u_int32_t st_maxnlockers;
	u_int32_t st_nobjects;
	u_int32_t st_maxnobjects;
	u_int32_t st_maxhobjects;
	uintmax_t st_objectsteals;
	uintmax_t st_maxosteals;
	uintmax_t st_nrequests;
	uintmax_t st_nreleases;
	uintmax_t st_nupgrade;
	uintmax_t st_ndowngrade;
	uintmax_t st_lock_wait;
	uintmax_t st_lock_nowait;
	uintmax_t st_ndeadlocks;
	db_timeout_t st_locktimeout;
	uintmax_t st_nlocktimeouts;
	db_timeout_t st_txntimeout;
	uintmax_t st_ntxntimeouts;
	uintmax_t st_part_wait;
	uintmax_t st_part_nowait;
	uintmax_t st_part_max_wait;
	uintmax_t st_part_max_nowait;
	uintmax_t st_objs_wait;
	uintmax_t st_objs_nowait;
	uintmax_t st_lockers_wait;
	uintmax_t st_lockers_nowait;
	uintmax_t st_region_wait;
	uintmax_t st_region_nowait;
	u_int32_t st_hash_len;
	roff_t	  st_regsize;
#endif
};
struct __db_lock_hstat {
	uintmax_t st_nrequests;
	uintmax_t st_nreleases;
	uintmax_t st_nupgrade;
	uintmax_t st_ndowngrade;
	u_int32_t st_nlocks;
	u_int32_t st_maxnlocks;
	u_int32_t st_nobjects;
	u_int32_t st_maxnobjects;
	uintmax_t st_lock_wait;
	uintmax_t st_lock_nowait;
	uintmax_t st_nlocktimeouts;
	uintmax_t st_ntxntimeouts;
	u_int32_t st_hash_len;
};
struct __db_lock_pstat {
	u_int32_t st_nlocks;
	u_int32_t st_maxnlocks;
	u_int32_t st_nobjects;
	u_int32_t st_maxnobjects;
	uintmax_t st_locksteals;
	uintmax_t st_objectsteals;
};
struct __db_ilock {
	db_pgno_t pgno;
	u_int8_t fileid[DB_FILE_ID_LEN];
#define	DB_HANDLE_LOCK	1
#define	DB_RECORD_LOCK	2
#define	DB_PAGE_LOCK	3
	u_int32_t type;
};
struct __db_lock_u {
	roff_t		off;
	u_int32_t	ndx;
	u_int32_t	gen;
	db_lockmode_t	mode;
};
struct __db_lockreq {
	db_lockop_t	 op;
	db_lockmode_t	 mode;
	db_timeout_t	 timeout;
	DBT		*obj;
	DB_LOCK		 lock;
};
#define	DB_LOGVERSION	16
#define DB_LOGVERSION_LATCHING 15
#define	DB_LOGCHKSUM	12
#define	DB_LOGOLDVER	8
#define	DB_LOGMAGIC	0x040988
struct __db_lsn {
	u_int32_t	file;
	u_int32_t	offset;
};
#define	DB_user_BEGIN		10000
#define	DB_debug_FLAG		0x80000000
struct __db_log_cursor {
	ENV	 *env;
	DB_FH	 *fhp;
	DB_LSN	  lsn;
	u_int32_t len;
	u_int32_t prev;
	DBT	  dbt;
	DB_LSN    p_lsn;
	u_int32_t p_version;
	u_int8_t *bp;
	u_int32_t bp_size;
	u_int32_t bp_rlen;
	DB_LSN	  bp_lsn;
	u_int32_t bp_maxrec;
	int (*close) __P((DB_LOGC *, u_int32_t));
	int (*get) __P((DB_LOGC *, DB_LSN *, DBT *, u_int32_t));
	int (*version) __P((DB_LOGC *, u_int32_t *, u_int32_t));
#define	DB_LOG_DISK		0x01
#define	DB_LOG_LOCKED		0x02
#define	DB_LOG_SILENT_ERR	0x04
	u_int32_t flags;
};
struct __db_log_stat {
	u_int32_t st_magic;
	u_int32_t st_version;
	int	  st_mode;
	u_int32_t st_lg_bsize;
	u_int32_t st_lg_size;
	u_int32_t st_wc_bytes;
	u_int32_t st_wc_mbytes;
#ifndef __TEST_DB_NO_STATISTICS
	uintmax_t st_record;
	u_int32_t st_w_bytes;
	u_int32_t st_w_mbytes;
	uintmax_t st_wcount;
	uintmax_t st_wcount_fill;
	uintmax_t st_rcount;
	uintmax_t st_scount;
	uintmax_t st_region_wait;
	uintmax_t st_region_nowait;
	u_int32_t st_cur_file;
	u_int32_t st_cur_offset;
	u_int32_t st_disk_file;
	u_int32_t st_disk_offset;
	u_int32_t st_maxcommitperflush;
	u_int32_t st_mincommitperflush;
	roff_t	  st_regsize;
#endif
};
#define	DB_SET_TXN_LSNP(txn, blsnp, llsnp)		\
	((txn)->set_txn_lsnp(txn, blsnp, llsnp))
typedef enum {
	DB_PRIORITY_UNCHANGED=0,
	DB_PRIORITY_VERY_LOW=1,
	DB_PRIORITY_LOW=2,
	DB_PRIORITY_DEFAULT=3,
	DB_PRIORITY_HIGH=4,
	DB_PRIORITY_VERY_HIGH=5
} DB_CACHE_PRIORITY;
struct __db_mpoolfile {
	DB_FH	  *fhp;
	u_int32_t  ref;
	u_int32_t pinref;
	struct {
		struct __db_mpoolfile *tqe_next;
		struct __db_mpoolfile **tqe_prev;
	} q;
	ENV	       *env;
	MPOOLFILE      *mfp;
	u_int32_t	clear_len;
	u_int8_t
			fileid[DB_FILE_ID_LEN];
	int		ftype;
	int32_t		lsn_offset;
	u_int32_t	gbytes, bytes;
	DBT	       *pgcookie;
	int32_t		priority;
	void	       *addr;
	size_t		len;
	u_int32_t	config_flags;
	int (*close) __P((DB_MPOOLFILE *, u_int32_t));
	int (*get)
	    __P((DB_MPOOLFILE *, db_pgno_t *, DB_TXN *, u_int32_t, void *));
	int (*get_clear_len) __P((DB_MPOOLFILE *, u_int32_t *));
	int (*get_fileid) __P((DB_MPOOLFILE *, u_int8_t *));
	int (*get_flags) __P((DB_MPOOLFILE *, u_int32_t *));
	int (*get_ftype) __P((DB_MPOOLFILE *, int *));
	int (*get_last_pgno) __P((DB_MPOOLFILE *, db_pgno_t *));
	int (*get_lsn_offset) __P((DB_MPOOLFILE *, int32_t *));
	int (*get_maxsize) __P((DB_MPOOLFILE *, u_int32_t *, u_int32_t *));
	int (*get_pgcookie) __P((DB_MPOOLFILE *, DBT *));
	int (*get_priority) __P((DB_MPOOLFILE *, DB_CACHE_PRIORITY *));
	int (*open) __P((DB_MPOOLFILE *, const char *, u_int32_t, int, size_t));
	int (*put) __P((DB_MPOOLFILE *, void *, DB_CACHE_PRIORITY, u_int32_t));
	int (*set_clear_len) __P((DB_MPOOLFILE *, u_int32_t));
	int (*set_fileid) __P((DB_MPOOLFILE *, u_int8_t *));
	int (*set_flags) __P((DB_MPOOLFILE *, u_int32_t, int));
	int (*set_ftype) __P((DB_MPOOLFILE *, int));
	int (*set_lsn_offset) __P((DB_MPOOLFILE *, int32_t));
	int (*set_maxsize) __P((DB_MPOOLFILE *, u_int32_t, u_int32_t));
	int (*set_pgcookie) __P((DB_MPOOLFILE *, DBT *));
	int (*set_priority) __P((DB_MPOOLFILE *, DB_CACHE_PRIORITY));
	int (*sync) __P((DB_MPOOLFILE *));
#define	MP_FILEID_SET	0x001
#define	MP_FLUSH	0x002
#define	MP_MULTIVERSION	0x004
#define	MP_OPEN_CALLED	0x008
#define	MP_READONLY	0x010
#define	MP_DUMMY	0x020
	u_int32_t  flags;
};
struct __db_mpool_stat {
	u_int32_t st_gbytes;
	u_int32_t st_bytes;
	u_int32_t st_ncache;
	u_int32_t st_max_ncache;
	size_t	  st_mmapsize;
	int	  st_maxopenfd;
	int	  st_maxwrite;
	db_timeout_t st_maxwrite_sleep;
	u_int32_t st_pages;
#ifndef __TEST_DB_NO_STATISTICS
	u_int32_t st_map;
	uintmax_t st_cache_hit;
	uintmax_t st_cache_miss;
	uintmax_t st_page_create;
	uintmax_t st_page_in;
	uintmax_t st_page_out;
	uintmax_t st_ro_evict;
	uintmax_t st_rw_evict;
	uintmax_t st_page_trickle;
	u_int32_t st_page_clean;
	u_int32_t st_page_dirty;
	u_int32_t st_hash_buckets;
	u_int32_t st_pagesize;
	u_int32_t st_hash_searches;
	u_int32_t st_hash_longest;
	uintmax_t st_hash_examined;
	uintmax_t st_hash_nowait;
	uintmax_t st_hash_wait;
	uintmax_t st_hash_max_nowait;
	uintmax_t st_hash_max_wait;
	uintmax_t st_region_nowait;
	uintmax_t st_region_wait;
	uintmax_t st_mvcc_frozen;
	uintmax_t st_mvcc_thawed;
	uintmax_t st_mvcc_freed;
	uintmax_t st_alloc;
	uintmax_t st_alloc_buckets;
	uintmax_t st_alloc_max_buckets;
	uintmax_t st_alloc_pages;
	uintmax_t st_alloc_max_pages;
	uintmax_t st_io_wait;
	uintmax_t st_sync_interrupted;
	roff_t	  st_regsize;
#endif
};
struct __db_mpool_fstat {
	char *file_name;
	u_int32_t st_pagesize;
#ifndef __TEST_DB_NO_STATISTICS
	u_int32_t st_map;
	uintmax_t st_cache_hit;
	uintmax_t st_cache_miss;
	uintmax_t st_page_create;
	uintmax_t st_page_in;
	uintmax_t st_page_out;
#endif
};
#define	DB_TXNVERSION	1
typedef enum {
	DB_TXN_ABORT=0,
	DB_TXN_APPLY=1,
	DB_TXN_BACKWARD_ROLL=3,
	DB_TXN_FORWARD_ROLL=4,
	DB_TXN_OPENFILES=5,
	DB_TXN_POPENFILES=6,
	DB_TXN_PRINT=7
} db_recops;
#define	DB_UNDO(op)	((op) == DB_TXN_ABORT || (op) == DB_TXN_BACKWARD_ROLL)
#define	DB_REDO(op)	((op) == DB_TXN_FORWARD_ROLL || (op) == DB_TXN_APPLY)
struct __db_txn {
	DB_TXNMGR	*mgrp;
	DB_TXN		*parent;
	DB_THREAD_INFO	*thread_info;
	u_int32_t	txnid;
	char		*name;
	DB_LOCKER	*locker;
	void		*td;
	db_timeout_t	lock_timeout;
	db_timeout_t	expire;
	void		*txn_list;
	struct {
		struct __db_txn *tqe_next;
		struct __db_txn **tqe_prev;
	} links;
	struct __kids {
		struct __db_txn *tqh_first;
		struct __db_txn **tqh_last;
	} kids;
	struct {
		struct __txn_event *tqh_first;
		struct __txn_event **tqh_last;
	} events;
	struct {
		struct __txn_logrec *stqh_first;
		struct __txn_logrec **stqh_last;
	} logs;
	struct {
		struct __db_txn *tqe_next;
		struct __db_txn **tqe_prev;
	} klinks;
	void	*api_internal;
	void	*xml_internal;
	u_int32_t	cursors;
	int	  (*abort) __P((DB_TXN *));
	int	  (*commit) __P((DB_TXN *, u_int32_t));
	int	  (*discard) __P((DB_TXN *, u_int32_t));
	int	  (*get_name) __P((DB_TXN *, const char **));
	u_int32_t (*id) __P((DB_TXN *));
	int	  (*prepare) __P((DB_TXN *, u_int8_t *));
	int	  (*set_name) __P((DB_TXN *, const char *));
	int	  (*set_timeout) __P((DB_TXN *, db_timeout_t, u_int32_t));
	void	  (*set_txn_lsnp) __P((DB_TXN *txn, DB_LSN **, DB_LSN **));
#define	TXN_CHILDCOMMIT		0x0001
#define	TXN_CDSGROUP		0x0002
#define	TXN_COMPENSATE		0x0004
#define	TXN_DEADLOCK		0x0008
#define	TXN_LOCKTIMEOUT		0x0010
#define	TXN_MALLOC		0x0020
#define	TXN_NOSYNC		0x0040
#define	TXN_NOWAIT		0x0080
#define	TXN_PRIVATE		0x0100
#define	TXN_READ_COMMITTED	0x0200
#define	TXN_READ_UNCOMMITTED	0x0400
#define	TXN_RESTORED		0x0800
#define	TXN_SNAPSHOT		0x1000
#define	TXN_SYNC		0x2000
#define	TXN_WRITE_NOSYNC	0x4000
	u_int32_t	flags;
};
#define	TXN_SYNC_FLAGS (TXN_SYNC | TXN_NOSYNC | TXN_WRITE_NOSYNC)
#define	DB_GID_SIZE	128
struct __db_preplist {
	DB_TXN	*txn;
	u_int8_t gid[DB_GID_SIZE];
};
struct __db_txn_active {
	u_int32_t txnid;
	u_int32_t parentid;
	pid_t     pid;
	db_threadid_t tid;
	DB_LSN	  lsn;
	DB_LSN	  read_lsn;
	u_int32_t mvcc_ref;
#define	TXN_ABORTED		1
#define	TXN_COMMITTED		2
#define	TXN_PREPARED		3
#define	TXN_RUNNING		4
	u_int32_t status;
	u_int8_t  gid[DB_GID_SIZE];
	char	  name[51];
};
struct __db_txn_stat {
	u_int32_t st_nrestores;
#ifndef __TEST_DB_NO_STATISTICS
	DB_LSN	  st_last_ckp;
	time_t	  st_time_ckp;
	u_int32_t st_last_txnid;
	u_int32_t st_maxtxns;
	uintmax_t st_naborts;
	uintmax_t st_nbegins;
	uintmax_t st_ncommits;
	u_int32_t st_nactive;
	u_int32_t st_nsnapshot;
	u_int32_t st_maxnactive;
	u_int32_t st_maxnsnapshot;
	DB_TXN_ACTIVE *st_txnarray;
	uintmax_t st_region_wait;
	uintmax_t st_region_nowait;
	roff_t	  st_regsize;
#endif
};
#define	DB_EID_BROADCAST	-1
#define	DB_EID_INVALID		-2
#define	DB_REP_DEFAULT_PRIORITY		100
#define	DB_REPMGR_ACKS_ALL		1
#define	DB_REPMGR_ACKS_ALL_PEERS	2
#define	DB_REPMGR_ACKS_NONE		3
#define	DB_REPMGR_ACKS_ONE		4
#define	DB_REPMGR_ACKS_ONE_PEER		5
#define	DB_REPMGR_ACKS_QUORUM		6
#define	DB_REP_ACK_TIMEOUT		1
#define	DB_REP_CHECKPOINT_DELAY		2
#define	DB_REP_CONNECTION_RETRY		3
#define	DB_REP_ELECTION_RETRY		4
#define	DB_REP_ELECTION_TIMEOUT		5
#define	DB_REP_FULL_ELECTION_TIMEOUT	6
#define	DB_REP_HEARTBEAT_MONITOR	7
#define	DB_REP_HEARTBEAT_SEND		8
#define	DB_REP_LEASE_TIMEOUT		9
#define	DB_EVENT_NO_SUCH_EVENT		 0
#define	DB_EVENT_PANIC			 1
#define	DB_EVENT_REG_ALIVE		 2
#define	DB_EVENT_REG_PANIC		 3
#define	DB_EVENT_REP_CLIENT		 4
#define	DB_EVENT_REP_ELECTED		 5
#define	DB_EVENT_REP_MASTER		 6
#define	DB_EVENT_REP_NEWMASTER		 7
#define	DB_EVENT_REP_PERM_FAILED	 8
#define	DB_EVENT_REP_STARTUPDONE	 9
#define	DB_EVENT_WRITE_FAILED		10
struct __db_repmgr_site {
	int eid;
	char *host;
	u_int port;
#define	DB_REPMGR_CONNECTED	0x01
#define	DB_REPMGR_DISCONNECTED	0x02
	u_int32_t status;
};
struct __db_rep_stat {
	uintmax_t st_log_queued;
	u_int32_t st_startup_complete;
#ifndef __TEST_DB_NO_STATISTICS
	u_int32_t st_status;
	DB_LSN st_next_lsn;
	DB_LSN st_waiting_lsn;
	DB_LSN st_max_perm_lsn;
	db_pgno_t st_next_pg;
	db_pgno_t st_waiting_pg;
	u_int32_t st_dupmasters;
	int st_env_id;
	u_int32_t st_env_priority;
	uintmax_t st_bulk_fills;
	uintmax_t st_bulk_overflows;
	uintmax_t st_bulk_records;
	uintmax_t st_bulk_transfers;
	uintmax_t st_client_rerequests;
	uintmax_t st_client_svc_req;
	uintmax_t st_client_svc_miss;
	u_int32_t st_gen;
	u_int32_t st_egen;
	uintmax_t st_log_duplicated;
	uintmax_t st_log_queued_max;
	uintmax_t st_log_queued_total;
	uintmax_t st_log_records;
	uintmax_t st_log_requested;
	int st_master;
	uintmax_t st_master_changes;
	uintmax_t st_msgs_badgen;
	uintmax_t st_msgs_processed;
	uintmax_t st_msgs_recover;
	uintmax_t st_msgs_send_failures;
	uintmax_t st_msgs_sent;
	uintmax_t st_newsites;
	u_int32_t st_nsites;
	uintmax_t st_nthrottles;
	uintmax_t st_outdated;
	uintmax_t st_pg_duplicated;
	uintmax_t st_pg_records;
	uintmax_t st_pg_requested;
	uintmax_t st_txns_applied;
	uintmax_t st_startsync_delayed;
	uintmax_t st_elections;
	uintmax_t st_elections_won;
	int st_election_cur_winner;
	u_int32_t st_election_gen;
	DB_LSN st_election_lsn;
	u_int32_t st_election_nsites;
	u_int32_t st_election_nvotes;
	u_int32_t st_election_priority;
	int st_election_status;
	u_int32_t st_election_tiebreaker;
	u_int32_t st_election_votes;
	u_int32_t st_election_sec;
	u_int32_t st_election_usec;
	u_int32_t st_max_lease_sec;
	u_int32_t st_max_lease_usec;
#ifdef	CONFIG_TEST
	u_int32_t st_filefail_cleanups;
#endif
#endif
};
struct __db_repmgr_stat {
	uintmax_t st_perm_failed;
	uintmax_t st_msgs_queued;
	uintmax_t st_msgs_dropped;
	uintmax_t st_connection_drop;
	uintmax_t st_connect_fail;
};
struct __db_seq_record {
	u_int32_t	seq_version;
	u_int32_t	flags;
	db_seq_t	seq_value;
	db_seq_t	seq_max;
	db_seq_t	seq_min;
};
struct __db_sequence {
	DB		*seq_dbp;
	db_mutex_t	mtx_seq;
	DB_SEQ_RECORD	*seq_rp;
	DB_SEQ_RECORD	seq_record;
	int32_t		seq_cache_size;
	db_seq_t	seq_last_value;
	DBT		seq_key;
	DBT		seq_data;
	void		*api_internal;
	int		(*close) __P((DB_SEQUENCE *, u_int32_t));
	int		(*get) __P((DB_SEQUENCE *,
			      DB_TXN *, int32_t, db_seq_t *, u_int32_t));
	int		(*get_cachesize) __P((DB_SEQUENCE *, int32_t *));
	int		(*get_db) __P((DB_SEQUENCE *, DB **));
	int		(*get_flags) __P((DB_SEQUENCE *, u_int32_t *));
	int		(*get_key) __P((DB_SEQUENCE *, DBT *));
	int		(*get_range) __P((DB_SEQUENCE *,
			     db_seq_t *, db_seq_t *));
	int		(*initial_value) __P((DB_SEQUENCE *, db_seq_t));
	int		(*open) __P((DB_SEQUENCE *,
			    DB_TXN *, DBT *, u_int32_t));
	int		(*remove) __P((DB_SEQUENCE *, DB_TXN *, u_int32_t));
	int		(*set_cachesize) __P((DB_SEQUENCE *, int32_t));
	int		(*set_flags) __P((DB_SEQUENCE *, u_int32_t));
	int		(*set_range) __P((DB_SEQUENCE *, db_seq_t, db_seq_t));
	int		(*stat) __P((DB_SEQUENCE *,
			    DB_SEQUENCE_STAT **, u_int32_t));
	int		(*stat_print) __P((DB_SEQUENCE *, u_int32_t));
};
struct __db_seq_stat {
	uintmax_t st_wait;
	uintmax_t st_nowait;
	db_seq_t  st_current;
	db_seq_t  st_value;
	db_seq_t  st_last_value;
	db_seq_t  st_min;
	db_seq_t  st_max;
	int32_t   st_cache_size;
	u_int32_t st_flags;
};
typedef enum {
	DB_BTREE=1,
	DB_HASH=2,
	DB_RECNO=3,
	DB_QUEUE=4,
	DB_UNKNOWN=5
} DBTYPE;
#define	DB_RENAMEMAGIC	0x030800
#define	DB_BTREEVERSION	9
#define	DB_BTREEOLDVER	8
#define	DB_BTREEMAGIC	0x053162
#define	DB_HASHVERSION	9
#define	DB_HASHOLDVER	7
#define	DB_HASHMAGIC	0x061561
#define	DB_QAMVERSION	4
#define	DB_QAMOLDVER	3
#define	DB_QAMMAGIC	0x042253
#define	DB_SEQUENCE_VERSION 2
#define	DB_SEQUENCE_OLDVER  1
#define	DB_AFTER		 1
#define	DB_APPEND		 2
#define	DB_BEFORE		 3
#define	DB_CONSUME		 4
#define	DB_CONSUME_WAIT		 5
#define	DB_CURRENT		 6
#define	DB_FIRST		 7
#define	DB_GET_BOTH		 8
#define	DB_GET_BOTHC		 9
#define	DB_GET_BOTH_RANGE	10
#define	DB_GET_RECNO		11
#define	DB_JOIN_ITEM		12
#define	DB_KEYFIRST		13
#define	DB_KEYLAST		14
#define	DB_LAST			15
#define	DB_NEXT			16
#define	DB_NEXT_DUP		17
#define	DB_NEXT_NODUP		18
#define	DB_NODUPDATA		19
#define	DB_NOOVERWRITE		20
#define	DB_NOSYNC		21
#define	DB_OVERWRITE_DUP	22
#define	DB_POSITION		23
#define	DB_PREV			24
#define	DB_PREV_DUP		25
#define	DB_PREV_NODUP		26
#define	DB_SET			27
#define	DB_SET_RANGE		28
#define	DB_SET_RECNO		29
#define	DB_UPDATE_SECONDARY	30
#define	DB_SET_LTE		31
#define	DB_GET_BOTH_LTE		32
#define	DB_OPFLAGS_MASK	0x000000ff
#define	DB_BUFFER_SMALL		(-30999)
#define	DB_DONOTINDEX		(-30998)
#define	DB_FOREIGN_CONFLICT	(-30997)
#define	DB_KEYEMPTY		(-30996)
#define	DB_KEYEXIST		(-30995)
#define	DB_LOCK_DEADLOCK	(-30994)
#define	DB_LOCK_NOTGRANTED	(-30993)
#define	DB_LOG_BUFFER_FULL	(-30992)
#define	DB_NOSERVER		(-30991)
#define	DB_NOSERVER_HOME	(-30990)
#define	DB_NOSERVER_ID		(-30989)
#define	DB_NOTFOUND		(-30988)
#define	DB_OLD_VERSION		(-30987)
#define	DB_PAGE_NOTFOUND	(-30986)
#define	DB_REP_DUPMASTER	(-30985)
#define	DB_REP_HANDLE_DEAD	(-30984)
#define	DB_REP_HOLDELECTION	(-30983)
#define	DB_REP_IGNORE		(-30982)
#define	DB_REP_ISPERM		(-30981)
#define	DB_REP_JOIN_FAILURE	(-30980)
#define	DB_REP_LEASE_EXPIRED	(-30979)
#define	DB_REP_LOCKOUT		(-30978)
#define	DB_REP_NEWSITE		(-30977)
#define	DB_REP_NOTPERM		(-30976)
#define	DB_REP_UNAVAIL		(-30975)
#define	DB_RUNRECOVERY		(-30974)
#define	DB_SECONDARY_BAD	(-30973)
#define	DB_VERIFY_BAD		(-30972)
#define	DB_VERSION_MISMATCH	(-30971)
#define	DB_ALREADY_ABORTED	(-30899)
#define	DB_DELETED		(-30898)
#define	DB_EVENT_NOT_HANDLED	(-30897)
#define	DB_NEEDSPLIT		(-30896)
#define	DB_REP_BULKOVF		(-30895)
#define	DB_REP_EGENCHG		(-30894)
#define	DB_REP_LOGREADY		(-30893)
#define	DB_REP_NEWMASTER	(-30892)
#define	DB_REP_PAGEDONE		(-30891)
#define	DB_REP_PAGELOCKED	(-30890)
#define	DB_SURPRISE_KID		(-30889)
#define	DB_SWAPBYTES		(-30888)
#define	DB_TIMEOUT		(-30887)
#define	DB_TXN_CKP		(-30886)
#define	DB_VERIFY_FATAL		(-30885)
struct __db {
	u_int32_t pgsize;
	DB_CACHE_PRIORITY priority;
	int (*db_append_recno) __P((DB *, DBT *, db_recno_t));
	void (*db_feedback) __P((DB *, int, int));
	int (*dup_compare) __P((DB *, const DBT *, const DBT *));
	void	*app_private;
	DB_ENV	*dbenv;
	ENV	*env;
	DBTYPE	 type;
	DB_MPOOLFILE *mpf;
	db_mutex_t mutex;
	char *fname, *dname;
	const char *dirname;
	u_int32_t open_flags;
	u_int8_t fileid[DB_FILE_ID_LEN];
	u_int32_t adj_fileid;
#define	DB_LOGFILEID_INVALID	-1
	FNAME *log_filename;
	db_pgno_t meta_pgno;
	DB_LOCKER *locker;
	DB_LOCKER *cur_locker;
	DB_TXN *cur_txn;
	DB_LOCKER *associate_locker;
	DB_LOCK	 handle_lock;
	u_int	 cl_id;
	time_t	 timestamp;
	u_int32_t fid_gen;
	DBT	 my_rskey;
	DBT	 my_rkey;
	DBT	 my_rdata;
	DB_FH	*saved_open_fhp;
	struct {
		struct __db *tqe_next;
		struct __db **tqe_prev;
	} dblistlinks;
	struct __cq_fq {
		struct __dbc *tqh_first;
		struct __dbc **tqh_last;
	} free_queue;
	struct __cq_aq {
		struct __dbc *tqh_first;
		struct __dbc **tqh_last;
	} active_queue;
	struct __cq_jq {
		struct __dbc *tqh_first;
		struct __dbc **tqh_last;
	} join_queue;
	struct {
		struct __db *lh_first;
	} s_secondaries;
	struct {
		struct __db *le_next;
		struct __db **le_prev;
	} s_links;
	u_int32_t s_refcnt;
	int	(*s_callback) __P((DB *, const DBT *, const DBT *, DBT *));
	DB	*s_primary;
#define	DB_ASSOC_IMMUTABLE_KEY    0x00000001
	u_int32_t s_assoc_flags;
	struct {
		struct __db_foreign_info *lh_first;
	} f_primaries;
	DB      *s_foreign;
	void	*api_internal;
	void	*bt_internal;
	void	*h_internal;
	void	*p_internal;
	void	*q_internal;
	int  (*associate) __P((DB *, DB_TXN *, DB *,
		int (*)(DB *, const DBT *, const DBT *, DBT *), u_int32_t));
	int  (*associate_foreign) __P((DB *, DB *,
		int (*)(DB *, const DBT *, DBT *, const DBT *, int *),
		u_int32_t));
	int  (*close) __P((DB *, u_int32_t));
	int  (*compact) __P((DB *,
		DB_TXN *, DBT *, DBT *, DB_COMPACT *, u_int32_t, DBT *));
	int  (*cursor) __P((DB *, DB_TXN *, DBC **, u_int32_t));
	int  (*del) __P((DB *, DB_TXN *, DBT *, u_int32_t));
	void (*err) __P((DB *, int, const char *, ...));
	void (*errx) __P((DB *, const char *, ...));
	int  (*exists) __P((DB *, DB_TXN *, DBT *, u_int32_t));
	int  (*fd) __P((DB *, int *));
	int  (*get) __P((DB *, DB_TXN *, DBT *, DBT *, u_int32_t));
	int  (*get_alloc) __P((DB *, void *(**)(size_t),
		void *(**)(void *, size_t), void (**)(void *)));
	int  (*get_append_recno) __P((DB *, int (**)(DB *, DBT *, db_recno_t)));
	int  (*get_bt_compare)
		__P((DB *, int (**)(DB *, const DBT *, const DBT *)));
	int  (*get_bt_compress) __P((DB *,
		int (**)(DB *,
		const DBT *, const DBT *, const DBT *, const DBT *, DBT *),
		int (**)(DB *, const DBT *, const DBT *, DBT *, DBT *, DBT *)));
	int  (*get_bt_minkey) __P((DB *, u_int32_t *));
	int  (*get_bt_prefix)
		__P((DB *, size_t (**)(DB *, const DBT *, const DBT *)));
	int  (*get_byteswapped) __P((DB *, int *));
	int  (*get_cachesize) __P((DB *, u_int32_t *, u_int32_t *, int *));
	int  (*get_create_dir) __P((DB *, const char **));
	int  (*get_dbname) __P((DB *, const char **, const char **));
	int  (*get_dup_compare)
		__P((DB *, int (**)(DB *, const DBT *, const DBT *)));
	int  (*get_encrypt_flags) __P((DB *, u_int32_t *));
	DB_ENV *(*get_env) __P((DB *));
	void (*get_errcall) __P((DB *,
		void (**)(const DB_ENV *, const char *, const char *)));
	void (*get_errfile) __P((DB *, FILE **));
	void (*get_errpfx) __P((DB *, const char **));
	int  (*get_feedback) __P((DB *, void (**)(DB *, int, int)));
	int  (*get_flags) __P((DB *, u_int32_t *));
	int  (*get_h_compare)
		__P((DB *, int (**)(DB *, const DBT *, const DBT *)));
	int  (*get_h_ffactor) __P((DB *, u_int32_t *));
	int  (*get_h_hash)
		__P((DB *, u_int32_t (**)(DB *, const void *, u_int32_t)));
	int  (*get_h_nelem) __P((DB *, u_int32_t *));
	int  (*get_lorder) __P((DB *, int *));
	DB_MPOOLFILE *(*get_mpf) __P((DB *));
	void (*get_msgcall) __P((DB *,
	    void (**)(const DB_ENV *, const char *)));
	void (*get_msgfile) __P((DB *, FILE **));
	int  (*get_multiple) __P((DB *));
	int  (*get_open_flags) __P((DB *, u_int32_t *));
	int  (*get_pagesize) __P((DB *, u_int32_t *));
	int  (*get_partition_callback) __P((DB *,
		u_int32_t *, u_int32_t (**)(DB *, DBT *key)));
	int  (*get_partition_dirs) __P((DB *, const char ***));
	int  (*get_partition_keys) __P((DB *, u_int32_t *, DBT **));
	int  (*get_priority) __P((DB *, DB_CACHE_PRIORITY *));
	int  (*get_q_extentsize) __P((DB *, u_int32_t *));
	int  (*get_re_delim) __P((DB *, int *));
	int  (*get_re_len) __P((DB *, u_int32_t *));
	int  (*get_re_pad) __P((DB *, int *));
	int  (*get_re_source) __P((DB *, const char **));
	int  (*get_transactional) __P((DB *));
	int  (*get_type) __P((DB *, DBTYPE *));
	int  (*join) __P((DB *, DBC **, DBC **, u_int32_t));
	int  (*key_range)
		__P((DB *, DB_TXN *, DBT *, DB_KEY_RANGE *, u_int32_t));
	int  (*open) __P((DB *,
		DB_TXN *, const char *, const char *, DBTYPE, u_int32_t, int));
	int  (*pget) __P((DB *, DB_TXN *, DBT *, DBT *, DBT *, u_int32_t));
	int  (*put) __P((DB *, DB_TXN *, DBT *, DBT *, u_int32_t));
	int  (*remove) __P((DB *, const char *, const char *, u_int32_t));
	int  (*rename) __P((DB *,
		const char *, const char *, const char *, u_int32_t));
	int  (*set_alloc) __P((DB *, void *(*)(size_t),
		void *(*)(void *, size_t), void (*)(void *)));
	int  (*set_append_recno) __P((DB *, int (*)(DB *, DBT *, db_recno_t)));
	int  (*set_bt_compare)
		__P((DB *, int (*)(DB *, const DBT *, const DBT *)));
	int  (*set_bt_compress) __P((DB *,
		int (*)(DB *, const DBT *, const DBT *, const DBT *, const DBT *, DBT *),
		int (*)(DB *, const DBT *, const DBT *, DBT *, DBT *, DBT *)));
	int  (*set_bt_minkey) __P((DB *, u_int32_t));
	int  (*set_bt_prefix)
		__P((DB *, size_t (*)(DB *, const DBT *, const DBT *)));
	int  (*set_cachesize) __P((DB *, u_int32_t, u_int32_t, int));
	int  (*set_create_dir) __P((DB *, const char *));
	int  (*set_dup_compare)
		__P((DB *, int (*)(DB *, const DBT *, const DBT *)));
	int  (*set_encrypt) __P((DB *, const char *, u_int32_t));
	void (*set_errcall) __P((DB *,
		void (*)(const DB_ENV *, const char *, const char *)));
	void (*set_errfile) __P((DB *, FILE *));
	void (*set_errpfx) __P((DB *, const char *));
	int  (*set_feedback) __P((DB *, void (*)(DB *, int, int)));
	int  (*set_flags) __P((DB *, u_int32_t));
	int  (*set_h_compare)
		__P((DB *, int (*)(DB *, const DBT *, const DBT *)));
	int  (*set_h_ffactor) __P((DB *, u_int32_t));
	int  (*set_h_hash)
		__P((DB *, u_int32_t (*)(DB *, const void *, u_int32_t)));
	int  (*set_h_nelem) __P((DB *, u_int32_t));
	int  (*set_lorder) __P((DB *, int));
	void (*set_msgcall) __P((DB *, void (*)(const DB_ENV *, const char *)));
	void (*set_msgfile) __P((DB *, FILE *));
	int  (*set_pagesize) __P((DB *, u_int32_t));
	int  (*set_paniccall) __P((DB *, void (*)(DB_ENV *, int)));
	int  (*set_partition) __P((DB *,
		u_int32_t, DBT *, u_int32_t (*)(DB *, DBT *key)));
	int  (*set_partition_dirs) __P((DB *, const char **));
	int  (*set_priority) __P((DB *, DB_CACHE_PRIORITY));
	int  (*set_q_extentsize) __P((DB *, u_int32_t));
	int  (*set_re_delim) __P((DB *, int));
	int  (*set_re_len) __P((DB *, u_int32_t));
	int  (*set_re_pad) __P((DB *, int));
	int  (*set_re_source) __P((DB *, const char *));
	int  (*sort_multiple) __P((DB *, DBT *, DBT *, u_int32_t));
	int  (*stat) __P((DB *, DB_TXN *, void *, u_int32_t));
	int  (*stat_print) __P((DB *, u_int32_t));
	int  (*sync) __P((DB *, u_int32_t));
	int  (*truncate) __P((DB *, DB_TXN *, u_int32_t *, u_int32_t));
	int  (*upgrade) __P((DB *, const char *, u_int32_t));
	int  (*verify)
		__P((DB *, const char *, const char *, FILE *, u_int32_t));
	int  (*dump) __P((DB *, const char *,
		int (*)(void *, const void *), void *, int, int));
	int  (*db_am_remove) __P((DB *, DB_THREAD_INFO *,
		DB_TXN *, const char *, const char *, u_int32_t));
	int  (*db_am_rename) __P((DB *, DB_THREAD_INFO *,
		DB_TXN *, const char *, const char *, const char *));
	int  (*stored_get) __P((DB *, DB_TXN *, DBT *, DBT *, u_int32_t));
	int  (*stored_close) __P((DB *, u_int32_t));
#define	DB_OK_BTREE	0x01
#define	DB_OK_HASH	0x02
#define	DB_OK_QUEUE	0x04
#define	DB_OK_RECNO	0x08
	u_int32_t	am_ok;
	int	 preserve_fid;
#define	DB_AM_CHKSUM		0x00000001
#define	DB_AM_COMPENSATE	0x00000002
#define	DB_AM_COMPRESS		0x00000004
#define	DB_AM_CREATED		0x00000008
#define	DB_AM_CREATED_MSTR	0x00000010
#define	DB_AM_DBM_ERROR		0x00000020
#define	DB_AM_DELIMITER		0x00000040
#define	DB_AM_DISCARD		0x00000080
#define	DB_AM_DUP		0x00000100
#define	DB_AM_DUPSORT		0x00000200
#define	DB_AM_ENCRYPT		0x00000400
#define	DB_AM_FIXEDLEN		0x00000800
#define	DB_AM_INMEM		0x00001000
#define	DB_AM_INORDER		0x00002000
#define	DB_AM_IN_RENAME		0x00004000
#define	DB_AM_NOT_DURABLE	0x00008000
#define	DB_AM_OPEN_CALLED	0x00010000
#define	DB_AM_PAD		0x00020000
#define	DB_AM_PGDEF		0x00040000
#define	DB_AM_RDONLY		0x00080000
#define	DB_AM_READ_UNCOMMITTED	0x00100000
#define	DB_AM_RECNUM		0x00200000
#define	DB_AM_RECOVER		0x00400000
#define	DB_AM_RENUMBER		0x00800000
#define	DB_AM_REVSPLITOFF	0x01000000
#define	DB_AM_SECONDARY		0x02000000
#define	DB_AM_SNAPSHOT		0x04000000
#define	DB_AM_SUBDB		0x08000000
#define	DB_AM_SWAP		0x10000000
#define	DB_AM_TXN		0x20000000
#define	DB_AM_VERIFYING		0x40000000
	u_int32_t orig_flags;
	u_int32_t flags;
};
#define	DB_MULTIPLE_INIT(pointer, dbt)					\
	(pointer = (u_int8_t *)(dbt)->data +				\
	    (dbt)->ulen - sizeof(u_int32_t))
#define	DB_MULTIPLE_NEXT(pointer, dbt, retdata, retdlen)		\
	do {								\
		u_int32_t *__p = (u_int32_t *)(pointer);		\
		if (*__p == (u_int32_t)-1) {				\
			retdata = NULL;					\
			pointer = NULL;					\
			break;						\
		}							\
		retdata = (u_int8_t *)(dbt)->data + *__p--;		\
		retdlen = *__p--;					\
		pointer = __p;						\
		if (retdlen == 0 && retdata == (u_int8_t *)(dbt)->data)	\
			retdata = NULL;					\
	} while (0)
#define	DB_MULTIPLE_KEY_NEXT(pointer, dbt, retkey, retklen, retdata, retdlen) \
	do {								\
		u_int32_t *__p = (u_int32_t *)(pointer);		\
		if (*__p == (u_int32_t)-1) {				\
			retdata = NULL;					\
			retkey = NULL;					\
			pointer = NULL;					\
			break;						\
		}							\
		retkey = (u_int8_t *)(dbt)->data + *__p--;		\
		retklen = *__p--;					\
		retdata = (u_int8_t *)(dbt)->data + *__p--;		\
		retdlen = *__p--;					\
		pointer = __p;						\
	} while (0)
#define	DB_MULTIPLE_RECNO_NEXT(pointer, dbt, recno, retdata, retdlen)   \
	do {								\
		u_int32_t *__p = (u_int32_t *)(pointer);		\
		if (*__p == (u_int32_t)0) {				\
			recno = 0;					\
			retdata = NULL;					\
			pointer = NULL;					\
			break;						\
		}							\
		recno = *__p--;						\
		retdata = (u_int8_t *)(dbt)->data + *__p--;		\
		retdlen = *__p--;					\
		pointer = __p;						\
	} while (0)
#define DB_MULTIPLE_WRITE_INIT(pointer, dbt)				\
	do {								\
		(dbt)->flags |= DB_DBT_BULK;				\
		pointer = (u_int8_t *)(dbt)->data +			\
		    (dbt)->ulen - sizeof(u_int32_t);			\
		*(u_int32_t *)(pointer) = (u_int32_t)-1;		\
	} while (0)
#define DB_MULTIPLE_RESERVE_NEXT(pointer, dbt, writedata, writedlen)	\
	do {								\
		u_int32_t *__p = (u_int32_t *)(pointer);		\
		u_int32_t __off = ((pointer) ==	(u_int8_t *)(dbt)->data +\
		    (dbt)->ulen - sizeof(u_int32_t)) ?  0 : __p[1] + __p[2];\
		if ((u_int8_t *)(dbt)->data + __off + (writedlen) >	\
		    (u_int8_t *)(__p - 2))				\
			writedata = NULL;				\
		else {							\
			writedata = (u_int8_t *)(dbt)->data + __off;	\
			__p[0] = __off;					\
			__p[-1] = (writedlen);				\
			__p[-2] = (u_int32_t)-1;			\
			pointer = __p - 2;				\
		}							\
	} while (0)
#define DB_MULTIPLE_WRITE_NEXT(pointer, dbt, writedata, writedlen)	\
	do {								\
		void *__destd;						\
		DB_MULTIPLE_RESERVE_NEXT((pointer), (dbt),		\
		    __destd, (writedlen));				\
		if (__destd == NULL)					\
			pointer = NULL;					\
		else							\
			memcpy(__destd, (writedata), (writedlen));	\
	} while (0)
#define DB_MULTIPLE_KEY_RESERVE_NEXT(pointer, dbt, writekey, writeklen, writedata, writedlen) \
	do {								\
		u_int32_t *__p = (u_int32_t *)(pointer);		\
		u_int32_t __off = ((pointer) == (u_int8_t *)(dbt)->data +\
		    (dbt)->ulen - sizeof(u_int32_t)) ?  0 : __p[1] + __p[2];\
		if ((u_int8_t *)(dbt)->data + __off + (writeklen) +	\
		    (writedlen) > (u_int8_t *)(__p - 4)) {		\
			writekey = NULL;				\
			writedata = NULL;				\
		} else {						\
			writekey = (u_int8_t *)(dbt)->data + __off;	\
			__p[0] = __off;					\
			__p[-1] = (writeklen);				\
			__p -= 2;					\
			__off += (writeklen);				\
			writedata = (u_int8_t *)(dbt)->data + __off;	\
			__p[0] = __off;					\
			__p[-1] = (writedlen);				\
			__p[-2] = (u_int32_t)-1;			\
			pointer = __p - 2;				\
		}							\
	} while (0)
#define DB_MULTIPLE_KEY_WRITE_NEXT(pointer, dbt, writekey, writeklen, writedata, writedlen) \
	do {								\
		void *__destk, *__destd;				\
		DB_MULTIPLE_KEY_RESERVE_NEXT((pointer), (dbt),		\
		    __destk, (writeklen), __destd, (writedlen));	\
		if (__destk == NULL)					\
			pointer = NULL;					\
		else {							\
			memcpy(__destk, (writekey), (writeklen));	\
			if (__destd != NULL)				\
				memcpy(__destd, (writedata), (writedlen));\
		}							\
	} while (0)
#define DB_MULTIPLE_RECNO_WRITE_INIT(pointer, dbt)			\
	do {								\
		(dbt)->flags |= DB_DBT_BULK;				\
		pointer = (u_int8_t *)(dbt)->data +			\
		    (dbt)->ulen - sizeof(u_int32_t);			\
		*(u_int32_t *)(pointer) = 0;				\
	} while (0)
#define DB_MULTIPLE_RECNO_RESERVE_NEXT(pointer, dbt, recno, writedata, writedlen) \
	do {								\
		u_int32_t *__p = (u_int32_t *)(pointer);		\
		u_int32_t __off = ((pointer) == (u_int8_t *)(dbt)->data +\
		    (dbt)->ulen - sizeof(u_int32_t)) ? 0 : __p[1] + __p[2]; \
		if (((u_int8_t *)(dbt)->data + __off) + (writedlen) >	\
		    (u_int8_t *)(__p - 3))				\
			writedata = NULL;				\
		else {							\
			writedata = (u_int8_t *)(dbt)->data + __off;	\
			__p[0] = (u_int32_t)(recno);			\
			__p[-1] = __off;				\
			__p[-2] = (writedlen);				\
			__p[-3] = 0;					\
			pointer = __p - 3;				\
		}							\
	} while (0)
#define DB_MULTIPLE_RECNO_WRITE_NEXT(pointer, dbt, recno, writedata, writedlen)\
	do {								\
		void *__destd;						\
		DB_MULTIPLE_RECNO_RESERVE_NEXT((pointer), (dbt),	\
		    (recno), __destd, (writedlen));			\
		if (__destd == NULL)					\
			pointer = NULL;					\
		else if ((writedlen) != 0)				\
			memcpy(__destd, (writedata), (writedlen));	\
	} while (0)
struct __dbc {
	DB *dbp;
	DB_ENV *dbenv;
	ENV *env;
	DB_THREAD_INFO *thread_info;
	DB_TXN	 *txn;
	DB_CACHE_PRIORITY priority;
	struct {
		DBC *tqe_next;
		DBC **tqe_prev;
	} links;
	DBT	 *rskey;
	DBT	 *rkey;
	DBT	 *rdata;
	DBT	  my_rskey;
	DBT	  my_rkey;
	DBT	  my_rdata;
	DB_LOCKER *lref;
	DB_LOCKER *locker;
	DBT	  lock_dbt;
	DB_LOCK_ILOCK lock;
	DB_LOCK	  mylock;
	u_int	  cl_id;
	DBTYPE	  dbtype;
	DBC_INTERNAL *internal;
	int (*close) __P((DBC *));
	int (*cmp) __P((DBC *, DBC *, int *, u_int32_t));
	int (*count) __P((DBC *, db_recno_t *, u_int32_t));
	int (*del) __P((DBC *, u_int32_t));
	int (*dup) __P((DBC *, DBC **, u_int32_t));
	int (*get) __P((DBC *, DBT *, DBT *, u_int32_t));
	int (*get_priority) __P((DBC *, DB_CACHE_PRIORITY *));
	int (*pget) __P((DBC *, DBT *, DBT *, DBT *, u_int32_t));
	int (*put) __P((DBC *, DBT *, DBT *, u_int32_t));
	int (*set_priority) __P((DBC *, DB_CACHE_PRIORITY));
	int (*c_close) __P((DBC *));
	int (*c_count) __P((DBC *, db_recno_t *, u_int32_t));
	int (*c_del) __P((DBC *, u_int32_t));
	int (*c_dup) __P((DBC *, DBC **, u_int32_t));
	int (*c_get) __P((DBC *, DBT *, DBT *, u_int32_t));
	int (*c_pget) __P((DBC *, DBT *, DBT *, DBT *, u_int32_t));
	int (*c_put) __P((DBC *, DBT *, DBT *, u_int32_t));
	int (*am_bulk) __P((DBC *, DBT *, u_int32_t));
	int (*am_close) __P((DBC *, db_pgno_t, int *));
	int (*am_del) __P((DBC *, u_int32_t));
	int (*am_destroy) __P((DBC *));
	int (*am_get) __P((DBC *, DBT *, DBT *, u_int32_t, db_pgno_t *));
	int (*am_put) __P((DBC *, DBT *, DBT *, u_int32_t, db_pgno_t *));
	int (*am_writelock) __P((DBC *));
#define	DBC_ACTIVE		0x00001
#define	DBC_BULK		0x00002
#define	DBC_DONTLOCK		0x00004
#define	DBC_DOWNREV		0x00008
#define	DBC_DUPLICATE		0x00010
#define	DBC_FROM_DB_GET		0x00020
#define	DBC_MULTIPLE		0x00040
#define	DBC_MULTIPLE_KEY	0x00080
#define	DBC_OPD			0x00100
#define	DBC_OWN_LID		0x00200
#define	DBC_PARTITIONED		0x00400
#define	DBC_READ_COMMITTED	0x00800
#define	DBC_READ_UNCOMMITTED	0x01000
#define	DBC_RECOVER		0x02000
#define	DBC_RMW			0x04000
#define	DBC_TRANSIENT		0x08000
#define	DBC_WAS_READ_COMMITTED	0x10000
#define	DBC_WRITECURSOR		0x20000
#define	DBC_WRITER		0x40000
	u_int32_t flags;
};
struct __key_range {
	double less;
	double equal;
	double greater;
};
struct __db_bt_stat {
	u_int32_t bt_magic;
	u_int32_t bt_version;
	u_int32_t bt_metaflags;
	u_int32_t bt_nkeys;
	u_int32_t bt_ndata;
	u_int32_t bt_pagecnt;
	u_int32_t bt_pagesize;
	u_int32_t bt_minkey;
	u_int32_t bt_re_len;
	u_int32_t bt_re_pad;
	u_int32_t bt_levels;
	u_int32_t bt_int_pg;
	u_int32_t bt_leaf_pg;
	u_int32_t bt_dup_pg;
	u_int32_t bt_over_pg;
	u_int32_t bt_empty_pg;
	u_int32_t bt_free;
	uintmax_t bt_int_pgfree;
	uintmax_t bt_leaf_pgfree;
	uintmax_t bt_dup_pgfree;
	uintmax_t bt_over_pgfree;
};
struct __db_compact {
	u_int32_t	compact_fillpercent;
	db_timeout_t	compact_timeout;
	u_int32_t	compact_pages;
	u_int32_t	compact_pages_free;
	u_int32_t	compact_pages_examine;
	u_int32_t	compact_levels;
	u_int32_t	compact_deadlock;
	db_pgno_t	compact_pages_truncated;
	db_pgno_t	compact_truncate;
};
struct __db_h_stat {
	u_int32_t hash_magic;
	u_int32_t hash_version;
	u_int32_t hash_metaflags;
	u_int32_t hash_nkeys;
	u_int32_t hash_ndata;
	u_int32_t hash_pagecnt;
	u_int32_t hash_pagesize;
	u_int32_t hash_ffactor;
	u_int32_t hash_buckets;
	u_int32_t hash_free;
	uintmax_t hash_bfree;
	u_int32_t hash_bigpages;
	uintmax_t hash_big_bfree;
	u_int32_t hash_overflows;
	uintmax_t hash_ovfl_free;
	u_int32_t hash_dup;
	uintmax_t hash_dup_free;
};
struct __db_qam_stat {
	u_int32_t qs_magic;
	u_int32_t qs_version;
	u_int32_t qs_metaflags;
	u_int32_t qs_nkeys;
	u_int32_t qs_ndata;
	u_int32_t qs_pagesize;
	u_int32_t qs_extentsize;
	u_int32_t qs_pages;
	u_int32_t qs_re_len;
	u_int32_t qs_re_pad;
	u_int32_t qs_pgfree;
	u_int32_t qs_first_recno;
	u_int32_t qs_cur_recno;
};
#define	DB_REGION_MAGIC	0x120897
struct __db_env {
	ENV *env;
	db_mutex_t mtx_db_env;
	void (*db_errcall) __P((const DB_ENV *, const char *, const char *));
	FILE		*db_errfile;
	const char	*db_errpfx;
	void (*db_msgcall) __P((const DB_ENV *, const char *));
	FILE		*db_msgfile;
	int   (*app_dispatch) __P((DB_ENV *, DBT *, DB_LSN *, db_recops));
	void  (*db_event_func) __P((DB_ENV *, u_int32_t, void *));
	void  (*db_feedback) __P((DB_ENV *, int, int));
	void  (*db_free) __P((void *));
	void  (*db_paniccall) __P((DB_ENV *, int));
	void *(*db_malloc) __P((size_t));
	void *(*db_realloc) __P((void *, size_t));
	int   (*is_alive) __P((DB_ENV *, pid_t, db_threadid_t, u_int32_t));
	void  (*thread_id) __P((DB_ENV *, pid_t *, db_threadid_t *));
	char *(*thread_id_string) __P((DB_ENV *, pid_t, db_threadid_t, char *));
	char	*db_log_dir;
	char	*db_tmp_dir;
	char    *db_create_dir;
	char   **db_data_dir;
	int	 data_cnt;
	int	 data_next;
	char	*intermediate_dir_mode;
	long	 shm_key;
	char	*passwd;
	size_t	 passwd_len;
	void	*cl_handle;
	u_int	 cl_id;
	void	*app_private;
	void	*api1_internal;
	void	*api2_internal;
	u_int32_t	verbose;
	u_int32_t	mutex_align;
	u_int32_t	mutex_cnt;
	u_int32_t	mutex_inc;
	u_int32_t	mutex_tas_spins;
	u_int8_t       *lk_conflicts;
	int		lk_modes;
	u_int32_t	lk_detect;
	u_int32_t	lk_max;
	u_int32_t	lk_max_lockers;
	u_int32_t	lk_max_objects;
	u_int32_t	lk_partitions ;
	db_timeout_t	lk_timeout;
	u_int32_t	lg_bsize;
	int		lg_filemode;
	u_int32_t	lg_regionmax;
	u_int32_t	lg_size;
	u_int32_t	lg_flags;
	u_int32_t	mp_gbytes;
	u_int32_t	mp_bytes;
	u_int32_t	mp_max_gbytes;
	u_int32_t	mp_max_bytes;
	size_t		mp_mmapsize;
	int		mp_maxopenfd;
	int		mp_maxwrite;
	u_int		mp_ncache;
	u_int32_t	mp_pagesize;
	u_int32_t	mp_tablesize;
	db_timeout_t	mp_maxwrite_sleep;
	u_int32_t	tx_max;
	time_t		tx_timestamp;
	db_timeout_t	tx_timeout;
	u_int32_t	thr_max;
	DB_FH		*registry;
	u_int32_t	registry_off;
        db_timeout_t	envreg_timeout;
#define	DB_ENV_AUTO_COMMIT	0x00000001
#define	DB_ENV_CDB_ALLDB	0x00000002
#define	DB_ENV_FAILCHK		0x00000004
#define	DB_ENV_DIRECT_DB	0x00000008
#define	DB_ENV_DSYNC_DB		0x00000010
#define	DB_ENV_MULTIVERSION	0x00000020
#define	DB_ENV_NOLOCKING	0x00000040
#define	DB_ENV_NOMMAP		0x00000080
#define	DB_ENV_NOPANIC		0x00000100
#define	DB_ENV_OVERWRITE	0x00000200
#define	DB_ENV_REGION_INIT	0x00000400
#define	DB_ENV_RPCCLIENT	0x00000800
#define	DB_ENV_RPCCLIENT_GIVEN	0x00001000
#define	DB_ENV_TIME_NOTGRANTED	0x00002000
#define	DB_ENV_TXN_NOSYNC	0x00004000
#define	DB_ENV_TXN_NOWAIT	0x00008000
#define	DB_ENV_TXN_SNAPSHOT	0x00010000
#define	DB_ENV_TXN_WRITE_NOSYNC	0x00020000
#define	DB_ENV_YIELDCPU		0x00040000
	u_int32_t flags;
	int  (*add_data_dir) __P((DB_ENV *, const char *));
	int  (*cdsgroup_begin) __P((DB_ENV *, DB_TXN **));
	int  (*close) __P((DB_ENV *, u_int32_t));
	int  (*dbremove) __P((DB_ENV *,
		DB_TXN *, const char *, const char *, u_int32_t));
	int  (*dbrename) __P((DB_ENV *,
		DB_TXN *, const char *, const char *, const char *, u_int32_t));
	void (*err) __P((const DB_ENV *, int, const char *, ...));
	void (*errx) __P((const DB_ENV *, const char *, ...));
	int  (*failchk) __P((DB_ENV *, u_int32_t));
	int  (*fileid_reset) __P((DB_ENV *, const char *, u_int32_t));
	int  (*get_alloc) __P((DB_ENV *, void *(**)(size_t),
		void *(**)(void *, size_t), void (**)(void *)));
	int  (*get_app_dispatch)
		__P((DB_ENV *, int (**)(DB_ENV *, DBT *, DB_LSN *, db_recops)));
	int  (*get_cache_max) __P((DB_ENV *, u_int32_t *, u_int32_t *));
	int  (*get_cachesize) __P((DB_ENV *, u_int32_t *, u_int32_t *, int *));
	int  (*get_create_dir) __P((DB_ENV *, const char **));
	int  (*get_data_dirs) __P((DB_ENV *, const char ***));
	int  (*get_encrypt_flags) __P((DB_ENV *, u_int32_t *));
	void (*get_errcall) __P((DB_ENV *,
		void (**)(const DB_ENV *, const char *, const char *)));
	void (*get_errfile) __P((DB_ENV *, FILE **));
	void (*get_errpfx) __P((DB_ENV *, const char **));
	int  (*get_flags) __P((DB_ENV *, u_int32_t *));
	int  (*get_feedback) __P((DB_ENV *, void (**)(DB_ENV *, int, int)));
	int  (*get_home) __P((DB_ENV *, const char **));
	int  (*get_intermediate_dir_mode) __P((DB_ENV *, const char **));
	int  (*get_isalive) __P((DB_ENV *,
		int (**)(DB_ENV *, pid_t, db_threadid_t, u_int32_t)));
	int  (*get_lg_bsize) __P((DB_ENV *, u_int32_t *));
	int  (*get_lg_dir) __P((DB_ENV *, const char **));
	int  (*get_lg_filemode) __P((DB_ENV *, int *));
	int  (*get_lg_max) __P((DB_ENV *, u_int32_t *));
	int  (*get_lg_regionmax) __P((DB_ENV *, u_int32_t *));
	int  (*get_lk_conflicts) __P((DB_ENV *, const u_int8_t **, int *));
	int  (*get_lk_detect) __P((DB_ENV *, u_int32_t *));
	int  (*get_lk_max_lockers) __P((DB_ENV *, u_int32_t *));
	int  (*get_lk_max_locks) __P((DB_ENV *, u_int32_t *));
	int  (*get_lk_max_objects) __P((DB_ENV *, u_int32_t *));
	int  (*get_lk_partitions) __P((DB_ENV *, u_int32_t *));
	int  (*get_mp_max_openfd) __P((DB_ENV *, int *));
	int  (*get_mp_max_write) __P((DB_ENV *, int *, db_timeout_t *));
	int  (*get_mp_mmapsize) __P((DB_ENV *, size_t *));
	int  (*get_mp_pagesize) __P((DB_ENV *, u_int32_t *));
	int  (*get_mp_tablesize) __P((DB_ENV *, u_int32_t *));
	void (*get_msgcall)
		__P((DB_ENV *, void (**)(const DB_ENV *, const char *)));
	void (*get_msgfile) __P((DB_ENV *, FILE **));
	int  (*get_open_flags) __P((DB_ENV *, u_int32_t *));
	int  (*get_shm_key) __P((DB_ENV *, long *));
	int  (*get_thread_count) __P((DB_ENV *, u_int32_t *));
	int  (*get_thread_id_fn)
		__P((DB_ENV *, void (**)(DB_ENV *, pid_t *, db_threadid_t *)));
	int  (*get_thread_id_string_fn) __P((DB_ENV *,
		char *(**)(DB_ENV *, pid_t, db_threadid_t, char *)));
	int  (*get_timeout) __P((DB_ENV *, db_timeout_t *, u_int32_t));
	int  (*get_tmp_dir) __P((DB_ENV *, const char **));
	int  (*get_tx_max) __P((DB_ENV *, u_int32_t *));
	int  (*get_tx_timestamp) __P((DB_ENV *, time_t *));
	int  (*get_verbose) __P((DB_ENV *, u_int32_t, int *));
	int  (*is_bigendian) __P((void));
	int  (*lock_detect) __P((DB_ENV *, u_int32_t, u_int32_t, int *));
	int  (*lock_get) __P((DB_ENV *,
		u_int32_t, u_int32_t, DBT *, db_lockmode_t, DB_LOCK *));
	int  (*lock_id) __P((DB_ENV *, u_int32_t *));
	int  (*lock_id_free) __P((DB_ENV *, u_int32_t));
	int  (*lock_put) __P((DB_ENV *, DB_LOCK *));
	int  (*lock_stat) __P((DB_ENV *, DB_LOCK_STAT **, u_int32_t));
	int  (*lock_stat_print) __P((DB_ENV *, u_int32_t));
	int  (*lock_vec) __P((DB_ENV *,
		u_int32_t, u_int32_t, DB_LOCKREQ *, int, DB_LOCKREQ **));
	int  (*log_archive) __P((DB_ENV *, char **[], u_int32_t));
	int  (*log_cursor) __P((DB_ENV *, DB_LOGC **, u_int32_t));
	int  (*log_file) __P((DB_ENV *, const DB_LSN *, char *, size_t));
	int  (*log_flush) __P((DB_ENV *, const DB_LSN *));
	int  (*log_get_config) __P((DB_ENV *, u_int32_t, int *));
	int  (*log_printf) __P((DB_ENV *, DB_TXN *, const char *, ...));
	int  (*log_put) __P((DB_ENV *, DB_LSN *, const DBT *, u_int32_t));
	int  (*log_set_config) __P((DB_ENV *, u_int32_t, int));
	int  (*log_stat) __P((DB_ENV *, DB_LOG_STAT **, u_int32_t));
	int  (*log_stat_print) __P((DB_ENV *, u_int32_t));
	int  (*lsn_reset) __P((DB_ENV *, const char *, u_int32_t));
	int  (*memp_fcreate) __P((DB_ENV *, DB_MPOOLFILE **, u_int32_t));
	int  (*memp_register) __P((DB_ENV *, int, int (*)(DB_ENV *, db_pgno_t,
		void *, DBT *), int (*)(DB_ENV *, db_pgno_t, void *, DBT *)));
	int  (*memp_stat) __P((DB_ENV *,
		DB_MPOOL_STAT **, DB_MPOOL_FSTAT ***, u_int32_t));
	int  (*memp_stat_print) __P((DB_ENV *, u_int32_t));
	int  (*memp_sync) __P((DB_ENV *, DB_LSN *));
	int  (*memp_trickle) __P((DB_ENV *, int, int *));
	int  (*mutex_alloc) __P((DB_ENV *, u_int32_t, db_mutex_t *));
	int  (*mutex_free) __P((DB_ENV *, db_mutex_t));
	int  (*mutex_get_align) __P((DB_ENV *, u_int32_t *));
	int  (*mutex_get_increment) __P((DB_ENV *, u_int32_t *));
	int  (*mutex_get_max) __P((DB_ENV *, u_int32_t *));
	int  (*mutex_get_tas_spins) __P((DB_ENV *, u_int32_t *));
	int  (*mutex_lock) __P((DB_ENV *, db_mutex_t));
	int  (*mutex_set_align) __P((DB_ENV *, u_int32_t));
	int  (*mutex_set_increment) __P((DB_ENV *, u_int32_t));
	int  (*mutex_set_max) __P((DB_ENV *, u_int32_t));
	int  (*mutex_set_tas_spins) __P((DB_ENV *, u_int32_t));
	int  (*mutex_stat) __P((DB_ENV *, DB_MUTEX_STAT **, u_int32_t));
	int  (*mutex_stat_print) __P((DB_ENV *, u_int32_t));
	int  (*mutex_unlock) __P((DB_ENV *, db_mutex_t));
	int  (*open) __P((DB_ENV *, const char *, u_int32_t, int));
	int  (*remove) __P((DB_ENV *, const char *, u_int32_t));
	int  (*rep_elect) __P((DB_ENV *, u_int32_t, u_int32_t, u_int32_t));
	int  (*rep_flush) __P((DB_ENV *));
	int  (*rep_get_clockskew) __P((DB_ENV *, u_int32_t *, u_int32_t *));
	int  (*rep_get_config) __P((DB_ENV *, u_int32_t, int *));
	int  (*rep_get_limit) __P((DB_ENV *, u_int32_t *, u_int32_t *));
	int  (*rep_get_nsites) __P((DB_ENV *, u_int32_t *));
	int  (*rep_get_priority) __P((DB_ENV *, u_int32_t *));
	int  (*rep_get_request) __P((DB_ENV *, u_int32_t *, u_int32_t *));
	int  (*rep_get_timeout) __P((DB_ENV *, int, u_int32_t *));
	int  (*rep_process_message)
		__P((DB_ENV *, DBT *, DBT *, int, DB_LSN *));
	int  (*rep_set_clockskew) __P((DB_ENV *, u_int32_t, u_int32_t));
	int  (*rep_set_config) __P((DB_ENV *, u_int32_t, int));
	int  (*rep_set_limit) __P((DB_ENV *, u_int32_t, u_int32_t));
	int  (*rep_set_nsites) __P((DB_ENV *, u_int32_t));
	int  (*rep_set_priority) __P((DB_ENV *, u_int32_t));
	int  (*rep_set_request) __P((DB_ENV *, u_int32_t, u_int32_t));
	int  (*rep_set_timeout) __P((DB_ENV *, int, db_timeout_t));
	int  (*rep_set_transport) __P((DB_ENV *, int, int (*)(DB_ENV *,
		const DBT *, const DBT *, const DB_LSN *, int, u_int32_t)));
	int  (*rep_start) __P((DB_ENV *, DBT *, u_int32_t));
	int  (*rep_stat) __P((DB_ENV *, DB_REP_STAT **, u_int32_t));
	int  (*rep_stat_print) __P((DB_ENV *, u_int32_t));
	int  (*rep_sync) __P((DB_ENV *, u_int32_t));
	int  (*repmgr_add_remote_site)
		__P((DB_ENV *, const char *, u_int, int *, u_int32_t));
	int  (*repmgr_get_ack_policy) __P((DB_ENV *, int *));
	int  (*repmgr_set_ack_policy) __P((DB_ENV *, int));
	int  (*repmgr_set_local_site)
		__P((DB_ENV *, const char *, u_int, u_int32_t));
	int  (*repmgr_site_list)
		__P((DB_ENV *, u_int *, DB_REPMGR_SITE **));
	int  (*repmgr_start) __P((DB_ENV *, int, u_int32_t));
	int  (*repmgr_stat) __P((DB_ENV *, DB_REPMGR_STAT **, u_int32_t));
	int  (*repmgr_stat_print) __P((DB_ENV *, u_int32_t));
	int  (*set_alloc) __P((DB_ENV *, void *(*)(size_t),
		void *(*)(void *, size_t), void (*)(void *)));
	int  (*set_app_dispatch)
		__P((DB_ENV *, int (*)(DB_ENV *, DBT *, DB_LSN *, db_recops)));
	int  (*set_cache_max) __P((DB_ENV *, u_int32_t, u_int32_t));
	int  (*set_cachesize) __P((DB_ENV *, u_int32_t, u_int32_t, int));
	int  (*set_create_dir) __P((DB_ENV *, const char *));
	int  (*set_data_dir) __P((DB_ENV *, const char *));
	int  (*set_encrypt) __P((DB_ENV *, const char *, u_int32_t));
	void (*set_errcall) __P((DB_ENV *,
		void (*)(const DB_ENV *, const char *, const char *)));
	void (*set_errfile) __P((DB_ENV *, FILE *));
	void (*set_errpfx) __P((DB_ENV *, const char *));
	int  (*set_event_notify)
		__P((DB_ENV *, void (*)(DB_ENV *, u_int32_t, void *)));
	int  (*set_feedback) __P((DB_ENV *, void (*)(DB_ENV *, int, int)));
	int  (*set_flags) __P((DB_ENV *, u_int32_t, int));
	int  (*set_intermediate_dir_mode) __P((DB_ENV *, const char *));
	int  (*set_isalive) __P((DB_ENV *,
		int (*)(DB_ENV *, pid_t, db_threadid_t, u_int32_t)));
	int  (*set_lg_bsize) __P((DB_ENV *, u_int32_t));
	int  (*set_lg_dir) __P((DB_ENV *, const char *));
	int  (*set_lg_filemode) __P((DB_ENV *, int));
	int  (*set_lg_max) __P((DB_ENV *, u_int32_t));
	int  (*set_lg_regionmax) __P((DB_ENV *, u_int32_t));
	int  (*set_lk_conflicts) __P((DB_ENV *, u_int8_t *, int));
	int  (*set_lk_detect) __P((DB_ENV *, u_int32_t));
	int  (*set_lk_max_lockers) __P((DB_ENV *, u_int32_t));
	int  (*set_lk_max_locks) __P((DB_ENV *, u_int32_t));
	int  (*set_lk_max_objects) __P((DB_ENV *, u_int32_t));
	int  (*set_lk_partitions) __P((DB_ENV *, u_int32_t));
	int  (*set_mp_max_openfd) __P((DB_ENV *, int));
	int  (*set_mp_max_write) __P((DB_ENV *, int, db_timeout_t));
	int  (*set_mp_mmapsize) __P((DB_ENV *, size_t));
	int  (*set_mp_pagesize) __P((DB_ENV *, u_int32_t));
	int  (*set_mp_tablesize) __P((DB_ENV *, u_int32_t));
	void (*set_msgcall)
		__P((DB_ENV *, void (*)(const DB_ENV *, const char *)));
	void (*set_msgfile) __P((DB_ENV *, FILE *));
	int  (*set_paniccall) __P((DB_ENV *, void (*)(DB_ENV *, int)));
	int  (*set_rpc_server)
		__P((DB_ENV *, void *, const char *, long, long, u_int32_t));
	int  (*set_shm_key) __P((DB_ENV *, long));
	int  (*set_thread_count) __P((DB_ENV *, u_int32_t));
	int  (*set_thread_id)
		__P((DB_ENV *, void (*)(DB_ENV *, pid_t *, db_threadid_t *)));
	int  (*set_thread_id_string) __P((DB_ENV *,
		char *(*)(DB_ENV *, pid_t, db_threadid_t, char *)));
	int  (*set_timeout) __P((DB_ENV *, db_timeout_t, u_int32_t));
	int  (*set_tmp_dir) __P((DB_ENV *, const char *));
	int  (*set_tx_max) __P((DB_ENV *, u_int32_t));
	int  (*set_tx_timestamp) __P((DB_ENV *, time_t *));
	int  (*set_verbose) __P((DB_ENV *, u_int32_t, int));
	int  (*stat_print) __P((DB_ENV *, u_int32_t));
	int  (*txn_begin) __P((DB_ENV *, DB_TXN *, DB_TXN **, u_int32_t));
	int  (*txn_checkpoint) __P((DB_ENV *, u_int32_t, u_int32_t, u_int32_t));
	int  (*txn_recover) __P((DB_ENV *,
		DB_PREPLIST *, u_int32_t, u_int32_t *, u_int32_t));
	int  (*txn_stat) __P((DB_ENV *, DB_TXN_STAT **, u_int32_t));
	int  (*txn_stat_print) __P((DB_ENV *, u_int32_t));
	int  (*prdbt) __P((DBT *,
		int, const char *, void *, int (*)(void *, const void *), int));
};
struct __db_distab {
	int   (**int_dispatch) __P((ENV *, DBT *, DB_LSN *, db_recops, void *));
	size_t	int_size;
	int   (**ext_dispatch) __P((DB_ENV *, DBT *, DB_LSN *, db_recops));
	size_t	ext_size;
};
#ifndef DB_DBM_HSEARCH
#define	DB_DBM_HSEARCH	0
#endif
#if DB_DBM_HSEARCH != 0
typedef struct __db DBM;
#define	DBM_INSERT	0
#define	DBM_REPLACE	1
#define	DBM_SUFFIX	".db"
#if defined(_XPG4_2)
typedef struct {
	char *dptr;
	size_t dsize;
} datum;
#else
typedef struct {
	char *dptr;
	int dsize;
} datum;
#endif
#define	dbm_clearerr(a)		__db_ndbm_clearerr(a)
#define	dbm_close(a)		__db_ndbm_close(a)
#define	dbm_delete(a, b)	__db_ndbm_delete(a, b)
#define	dbm_dirfno(a)		__db_ndbm_dirfno(a)
#define	dbm_error(a)		__db_ndbm_error(a)
#define	dbm_fetch(a, b)		__db_ndbm_fetch(a, b)
#define	dbm_firstkey(a)		__db_ndbm_firstkey(a)
#define	dbm_nextkey(a)		__db_ndbm_nextkey(a)
#define	dbm_open(a, b, c)	__db_ndbm_open(a, b, c)
#define	dbm_pagfno(a)		__db_ndbm_pagfno(a)
#define	dbm_rdonly(a)		__db_ndbm_rdonly(a)
#define	dbm_store(a, b, c, d) \
	__db_ndbm_store(a, b, c, d)
#define	dbminit(a)	__db_dbm_init(a)
#define	dbmclose	__db_dbm_close
#if !defined(__cplusplus)
#define	delete(a)	__db_dbm_delete(a)
#endif
#define	fetch(a)	__db_dbm_fetch(a)
#define	firstkey	__db_dbm_firstkey
#define	nextkey(a)	__db_dbm_nextkey(a)
#define	store(a, b)	__db_dbm_store(a, b)
typedef enum {
	FIND, ENTER
} ACTION;
typedef struct entry {
	char *key;
	char *data;
} ENTRY;
#define	hcreate(a)	__db_hcreate(a)
#define	hdestroy	__db_hdestroy
#define	hsearch(a, b)	__db_hsearch(a, b)
#endif
#if defined(__cplusplus)
}
#endif
#ifdef _MSC_VER
#pragma warning(pop)
#endif
#endif
#define	DB_AGGRESSIVE				0x00000001
#define	DB_ARCH_ABS				0x00000001
#define	DB_ARCH_DATA				0x00000002
#define	DB_ARCH_LOG				0x00000004
#define	DB_ARCH_REMOVE				0x00000008
#define	DB_AUTO_COMMIT				0x00000100
#define	DB_CDB_ALLDB				0x00000040
#define	DB_CHKSUM				0x00000008
#define	DB_CKP_INTERNAL				0x00000002
#define	DB_CREATE				0x00000001
#define	DB_CURSOR_BULK				0x00000001
#define	DB_CURSOR_TRANSIENT			0x00000004
#define	DB_CXX_NO_EXCEPTIONS			0x00000002
#define	DB_DIRECT				0x00000010
#define	DB_DIRECT_DB				0x00000080
#define	DB_DSYNC_DB				0x00000200
#define	DB_DUP					0x00000010
#define	DB_DUPSORT				0x00000004
#define	DB_DURABLE_UNKNOWN			0x00000020
#define	DB_ENCRYPT				0x00000001
#define	DB_ENCRYPT_AES				0x00000001
#define	DB_EXCL					0x00000040
#define	DB_EXTENT				0x00000040
#define	DB_FAILCHK				0x00000020
#define	DB_FAST_STAT				0x00000001
#define	DB_FCNTL_LOCKING			0x00000800
#define	DB_FLUSH				0x00000001
#define	DB_FORCE				0x00000001
#define	DB_FOREIGN_ABORT			0x00000001
#define	DB_FOREIGN_CASCADE			0x00000002
#define	DB_FOREIGN_NULLIFY			0x00000004
#define	DB_FREELIST_ONLY			0x00000001
#define	DB_FREE_SPACE				0x00000002
#define	DB_IGNORE_LEASE				0x00002000
#define	DB_IMMUTABLE_KEY			0x00000002
#define	DB_INIT_CDB				0x00000040
#define	DB_INIT_LOCK				0x00000080
#define	DB_INIT_LOG				0x00000100
#define	DB_INIT_MPOOL				0x00000200
#define	DB_INIT_REP				0x00000400
#define	DB_INIT_TXN				0x00000800
#define	DB_INORDER				0x00000020
#define	DB_JOIN_NOSORT				0x00000001
#define	DB_LOCKDOWN				0x00001000
#define	DB_LOCK_NOWAIT				0x00000001
#define	DB_LOCK_RECORD				0x00000002
#define	DB_LOCK_SET_TIMEOUT			0x00000004
#define	DB_LOCK_SWITCH				0x00000008
#define	DB_LOCK_UPGRADE				0x00000010
#define	DB_LOG_AUTO_REMOVE			0x00000001
#define	DB_LOG_CHKPNT				0x00000002
#define	DB_LOG_COMMIT				0x00000004
#define	DB_LOG_DIRECT				0x00000002
#define	DB_LOG_DSYNC				0x00000004
#define	DB_LOG_IN_MEMORY			0x00000008
#define	DB_LOG_NOCOPY				0x00000008
#define	DB_LOG_NOT_DURABLE			0x00000010
#define	DB_LOG_WRNOSYNC				0x00000020
#define	DB_LOG_ZERO				0x00000010
#define	DB_MPOOL_CREATE				0x00000001
#define	DB_MPOOL_DIRTY				0x00000002
#define	DB_MPOOL_DISCARD			0x00000001
#define	DB_MPOOL_EDIT				0x00000004
#define	DB_MPOOL_FREE				0x00000008
#define	DB_MPOOL_LAST				0x00000010
#define	DB_MPOOL_NEW				0x00000020
#define	DB_MPOOL_NOFILE				0x00000001
#define	DB_MPOOL_NOLOCK				0x00000002
#define	DB_MPOOL_TRY				0x00000040
#define	DB_MPOOL_UNLINK				0x00000002
#define	DB_MULTIPLE				0x00000800
#define	DB_MULTIPLE_KEY				0x00004000
#define	DB_MULTIVERSION				0x00000004
#define	DB_MUTEX_ALLOCATED			0x00000001
#define	DB_MUTEX_LOCKED				0x00000002
#define	DB_MUTEX_LOGICAL_LOCK			0x00000004
#define	DB_MUTEX_PROCESS_ONLY			0x00000008
#define	DB_MUTEX_SELF_BLOCK			0x00000010
#define	DB_MUTEX_SHARED				0x00000020
#define	DB_NOLOCKING				0x00000400
#define	DB_NOMMAP				0x00000008
#define	DB_NOORDERCHK				0x00000002
#define	DB_NOPANIC				0x00000800
#define	DB_NO_AUTO_COMMIT			0x00001000
#define	DB_ODDFILESIZE				0x00000080
#define	DB_ORDERCHKONLY				0x00000004
#define	DB_OVERWRITE				0x00001000
#define	DB_PANIC_ENVIRONMENT			0x00002000
#define	DB_PRINTABLE				0x00000008
#define	DB_PRIVATE				0x00002000
#define	DB_PR_PAGE				0x00000010
#define	DB_PR_RECOVERYTEST			0x00000020
#define	DB_RDONLY				0x00000400
#define	DB_RDWRMASTER				0x00002000
#define	DB_READ_COMMITTED			0x00000400
#define	DB_READ_UNCOMMITTED			0x00000200
#define	DB_RECNUM				0x00000040
#define	DB_RECOVER				0x00000002
#define	DB_RECOVER_FATAL			0x00004000
#define	DB_REGION_INIT				0x00004000
#define	DB_REGISTER				0x00008000
#define	DB_RENUMBER				0x00000080
#define	DB_REPMGR_CONF_2SITE_STRICT		0x00000001
#define	DB_REPMGR_PEER				0x00000001
#define	DB_REP_ANYWHERE				0x00000001
#define	DB_REP_CLIENT				0x00000001
#define	DB_REP_CONF_BULK			0x00000002
#define	DB_REP_CONF_DELAYCLIENT			0x00000004
#define	DB_REP_CONF_INMEM			0x00000008
#define	DB_REP_CONF_LEASE			0x00000010
#define	DB_REP_CONF_NOAUTOINIT			0x00000020
#define	DB_REP_CONF_NOWAIT			0x00000040
#define	DB_REP_ELECTION				0x00000004
#define	DB_REP_MASTER				0x00000002
#define	DB_REP_NOBUFFER				0x00000002
#define	DB_REP_PERMANENT			0x00000004
#define	DB_REP_REREQUEST			0x00000008
#define	DB_REVSPLITOFF				0x00000100
#define	DB_RMW					0x00001000
#define	DB_RPCCLIENT				0x00000001
#define	DB_SALVAGE				0x00000040
#define	DB_SA_SKIPFIRSTKEY			0x00000080
#define	DB_SA_UNKNOWNKEY			0x00000100
#define	DB_SEQ_DEC				0x00000001
#define	DB_SEQ_INC				0x00000002
#define	DB_SEQ_RANGE_SET			0x00000004
#define	DB_SEQ_WRAP				0x00000008
#define	DB_SEQ_WRAPPED				0x00000010
#define	DB_SET_LOCK_TIMEOUT			0x00000001
#define	DB_SET_REG_TIMEOUT			0x00000004
#define	DB_SET_TXN_NOW				0x00000008
#define	DB_SET_TXN_TIMEOUT			0x00000002
#define	DB_SHALLOW_DUP				0x00000100
#define	DB_SNAPSHOT				0x00000200
#define	DB_STAT_ALL				0x00000004
#define	DB_STAT_CLEAR				0x00000001
#define	DB_STAT_LOCK_CONF			0x00000008
#define	DB_STAT_LOCK_LOCKERS			0x00000010
#define	DB_STAT_LOCK_OBJECTS			0x00000020
#define	DB_STAT_LOCK_PARAMS			0x00000040
#define	DB_STAT_MEMP_HASH			0x00000008
#define	DB_STAT_MEMP_NOERROR			0x00000010
#define	DB_STAT_SUBSYSTEM			0x00000002
#define	DB_ST_DUPOK				0x00000200
#define	DB_ST_DUPSET				0x00000400
#define	DB_ST_DUPSORT				0x00000800
#define	DB_ST_IS_RECNO				0x00001000
#define	DB_ST_OVFL_LEAF				0x00002000
#define	DB_ST_RECNUM				0x00004000
#define	DB_ST_RELEN				0x00008000
#define	DB_ST_TOPLEVEL				0x00010000
#define	DB_SYSTEM_MEM				0x00010000
#define	DB_THREAD				0x00000010
#define	DB_TIME_NOTGRANTED			0x00008000
#define	DB_TRUNCATE				0x00004000
#define	DB_TXN_NOSYNC				0x00000001
#define	DB_TXN_NOT_DURABLE			0x00000002
#define	DB_TXN_NOWAIT				0x00000010
#define	DB_TXN_SNAPSHOT				0x00000002
#define	DB_TXN_SYNC				0x00000004
#define	DB_TXN_WAIT				0x00000008
#define	DB_TXN_WRITE_NOSYNC			0x00000020
#define	DB_UNREF				0x00020000
#define	DB_UPGRADE				0x00000001
#define	DB_USE_ENVIRON				0x00000004
#define	DB_USE_ENVIRON_ROOT			0x00000008
#define	DB_VERB_DEADLOCK			0x00000001
#define	DB_VERB_FILEOPS				0x00000002
#define	DB_VERB_FILEOPS_ALL			0x00000004
#define	DB_VERB_RECOVERY			0x00000008
#define	DB_VERB_REGISTER			0x00000010
#define	DB_VERB_REPLICATION			0x00000020
#define	DB_VERB_REPMGR_CONNFAIL			0x00000040
#define	DB_VERB_REPMGR_MISC			0x00000080
#define	DB_VERB_REP_ELECT			0x00000100
#define	DB_VERB_REP_LEASE			0x00000200
#define	DB_VERB_REP_MISC			0x00000400
#define	DB_VERB_REP_MSGS			0x00000800
#define	DB_VERB_REP_SYNC			0x00001000
#define	DB_VERB_REP_TEST			0x00002000
#define	DB_VERB_WAITSFOR			0x00004000
#define	DB_VERIFY				0x00000002
#define	DB_VERIFY_PARTITION			0x00040000
#define	DB_WRITECURSOR				0x00000008
#define	DB_WRITELOCK				0x00000010
#define	DB_WRITEOPEN				0x00008000
#define	DB_YIELDCPU				0x00010000
#ifndef	_DB_EXT_PROT_IN_
#define	_DB_EXT_PROT_IN_
#if defined(__cplusplus)
extern "C" {
#endif
int db_create __P((DB **, DB_ENV *, u_int32_t));
char *db_strerror __P((int));
int db_env_set_func_close __P((int (*)(int)));
int db_env_set_func_dirfree __P((void (*)(char **, int)));
int db_env_set_func_dirlist __P((int (*)(const char *, char ***, int *)));
int db_env_set_func_exists __P((int (*)(const char *, int *)));
int db_env_set_func_free __P((void (*)(void *)));
int db_env_set_func_fsync __P((int (*)(int)));
int db_env_set_func_ftruncate __P((int (*)(int, off_t)));
int db_env_set_func_ioinfo __P((int (*)(const char *, int, u_int32_t *, u_int32_t *, u_int32_t *)));
int db_env_set_func_malloc __P((void *(*)(size_t)));
int db_env_set_func_file_map __P((int (*)(DB_ENV *, char *, size_t, int, void **), int (*)(DB_ENV *, void *)));
int db_env_set_func_region_map __P((int (*)(DB_ENV *, char *, size_t, int *, void **), int (*)(DB_ENV *, void *)));
int db_env_set_func_pread __P((ssize_t (*)(int, void *, size_t, off_t)));
int db_env_set_func_pwrite __P((ssize_t (*)(int, const void *, size_t, off_t)));
int db_env_set_func_open __P((int (*)(const char *, int, ...)));
int db_env_set_func_read __P((ssize_t (*)(int, void *, size_t)));
int db_env_set_func_realloc __P((void *(*)(void *, size_t)));
int db_env_set_func_rename __P((int (*)(const char *, const char *)));
int db_env_set_func_seek __P((int (*)(int, off_t, int)));
int db_env_set_func_unlink __P((int (*)(const char *)));
int db_env_set_func_write __P((ssize_t (*)(int, const void *, size_t)));
int db_env_set_func_yield __P((int (*)(u_long, u_long)));
int db_env_create __P((DB_ENV **, u_int32_t));
char *db_version __P((int *, int *, int *));
int log_compare __P((const DB_LSN *, const DB_LSN *));
int db_sequence_create __P((DB_SEQUENCE **, DB *, u_int32_t));
#if DB_DBM_HSEARCH != 0
int	 __db_ndbm_clearerr __P((DBM *));
void	 __db_ndbm_close __P((DBM *));
int	 __db_ndbm_delete __P((DBM *, datum));
int	 __db_ndbm_dirfno __P((DBM *));
int	 __db_ndbm_error __P((DBM *));
datum __db_ndbm_fetch __P((DBM *, datum));
datum __db_ndbm_firstkey __P((DBM *));
datum __db_ndbm_nextkey __P((DBM *));
DBM	*__db_ndbm_open __P((const char *, int, int));
int	 __db_ndbm_pagfno __P((DBM *));
int	 __db_ndbm_rdonly __P((DBM *));
int	 __db_ndbm_store __P((DBM *, datum, datum, int));
int	 __db_dbm_close __P((void));
int	 __db_dbm_delete __P((datum));
datum __db_dbm_fetch __P((datum));
datum __db_dbm_firstkey __P((void));
int	 __db_dbm_init __P((char *));
datum __db_dbm_nextkey __P((datum));
int	 __db_dbm_store __P((datum, datum));
#endif
#if DB_DBM_HSEARCH != 0
int __db_hcreate __P((size_t));
ENTRY *__db_hsearch __P((ENTRY, ACTION));
void __db_hdestroy __P((void));
#endif
#if defined(__cplusplus)
}
#endif
#endif
