#ifndef _DB_CXX_H_
#define	_DB_CXX_H_
#include <stdarg.h>
#define	HAVE_CXX_STDHEADERS 1
#ifdef HAVE_CXX_STDHEADERS
#include <iostream>
#include <exception>
#define	__DB_STD(x)	std::x
#else
#include <iostream.h>
#include <exception.h>
#define	__DB_STD(x)	x
#endif
#include "db.h"
class Db;
class Dbc;
class DbEnv;
class DbInfo;
class DbLock;
class DbLogc;
class DbLsn;
class DbMpoolFile;
class DbPreplist;
class DbSequence;
class Dbt;
class DbTxn;
class DbMultipleIterator;
class DbMultipleKeyDataIterator;
class DbMultipleRecnoDataIterator;
class DbMultipleDataIterator;
class DbException;
class DbDeadlockException;
class DbLockNotGrantedException;
class DbMemoryException;
class DbRepHandleDeadException;
class DbRunRecoveryException;
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4201 4514)
#endif
#if defined(_MSC_VER)
#  if defined(DB_CREATE_DLL)
#    define _exported __declspec(dllexport)
#  elif defined(DB_USE_DLL)
#    define _exported __declspec(dllimport)
#  else
#    define _exported
#  endif
#else
#  define _exported
#endif
extern "C" {
	typedef void * (*db_malloc_fcn_type)
		(size_t);
	typedef void * (*db_realloc_fcn_type)
		(void *, size_t);
	typedef void (*db_free_fcn_type)
		(void *);
	typedef int (*bt_compare_fcn_type)
		(DB *, const DBT *, const DBT *);
	typedef size_t (*bt_prefix_fcn_type)
		(DB *, const DBT *, const DBT *);
	typedef int (*dup_compare_fcn_type)
		(DB *, const DBT *, const DBT *);
	typedef int (*h_compare_fcn_type)
		(DB *, const DBT *, const DBT *);
	typedef u_int32_t (*h_hash_fcn_type)
		(DB *, const void *, u_int32_t);
	typedef int (*pgin_fcn_type)
		(DB_ENV *dbenv, db_pgno_t pgno, void *pgaddr, DBT *pgcookie);
	typedef int (*pgout_fcn_type)
		(DB_ENV *dbenv, db_pgno_t pgno, void *pgaddr, DBT *pgcookie);
}
class _exported Db
{
	friend class DbEnv;
public:
	Db(DbEnv*, u_int32_t);
	virtual ~Db();
	virtual int associate(DbTxn *txn, Db *secondary, int (*callback)
	    (Db *, const Dbt *, const Dbt *, Dbt *), u_int32_t flags);
	virtual int associate_foreign(Db *foreign, int (*callback)
	    (Db *, const Dbt *, Dbt *, const Dbt *, int *), u_int32_t flags);
	virtual int close(u_int32_t flags);
	virtual int compact(DbTxn *txnid, Dbt *start,
	    Dbt *stop, DB_COMPACT *c_data, u_int32_t flags, Dbt *end);
	virtual int cursor(DbTxn *txnid, Dbc **cursorp, u_int32_t flags);
	virtual int del(DbTxn *txnid, Dbt *key, u_int32_t flags);
	virtual void err(int, const char *, ...);
	virtual void errx(const char *, ...);
	virtual int exists(DbTxn *txnid, Dbt *key, u_int32_t flags);
	virtual int fd(int *fdp);
	virtual int get(DbTxn *txnid, Dbt *key, Dbt *data, u_int32_t flags);
	virtual int get_alloc(
	    db_malloc_fcn_type *, db_realloc_fcn_type *, db_free_fcn_type *);
	virtual int get_append_recno(int (**)(Db *, Dbt *, db_recno_t));
	virtual int get_bt_compare(int (**)(Db *, const Dbt *, const Dbt *));
	virtual int get_bt_compress(
	    int (**)(
	    Db *, const Dbt *, const Dbt *, const Dbt *, const Dbt *, Dbt *),
	    int (**)(Db *, const Dbt *, const Dbt *, Dbt *, Dbt *, Dbt *));
	virtual int get_bt_minkey(u_int32_t *);
	virtual int get_bt_prefix(size_t (**)(Db *, const Dbt *, const Dbt *));
	virtual int get_byteswapped(int *);
	virtual int get_cachesize(u_int32_t *, u_int32_t *, int *);
	virtual int get_create_dir(const char **);
	virtual int get_dbname(const char **, const char **);
	virtual int get_dup_compare(int (**)(Db *, const Dbt *, const Dbt *));
	virtual int get_encrypt_flags(u_int32_t *);
	virtual void get_errcall(
	    void (**)(const DbEnv *, const char *, const char *));
	virtual void get_errfile(FILE **);
	virtual void get_errpfx(const char **);
	virtual int get_feedback(void (**)(Db *, int, int));
	virtual int get_flags(u_int32_t *);
	virtual int get_h_compare(int (**)(Db *, const Dbt *, const Dbt *));
	virtual int get_h_ffactor(u_int32_t *);
	virtual int get_h_hash(u_int32_t (**)(Db *, const void *, u_int32_t));
	virtual int get_h_nelem(u_int32_t *);
	virtual int get_lorder(int *);
	virtual void get_msgcall(void (**)(const DbEnv *, const char *));
	virtual void get_msgfile(FILE **);
	virtual int get_multiple();
	virtual int get_open_flags(u_int32_t *);
	virtual int get_pagesize(u_int32_t *);
	virtual int get_partition_callback(
	    u_int32_t *, u_int32_t (**)(Db *, Dbt *key));
	virtual int get_partition_dirs(const char ***);
	virtual int get_partition_keys(u_int32_t *, Dbt **);
	virtual int get_priority(DB_CACHE_PRIORITY *);
	virtual int get_q_extentsize(u_int32_t *);
	virtual int get_re_delim(int *);
	virtual int get_re_len(u_int32_t *);
	virtual int get_re_pad(int *);
	virtual int get_re_source(const char **);
	virtual int get_transactional();
	virtual int get_type(DBTYPE *);
	virtual int join(Dbc **curslist, Dbc **dbcp, u_int32_t flags);
	virtual int key_range(DbTxn *, Dbt *, DB_KEY_RANGE *, u_int32_t);
	virtual int open(DbTxn *txnid,
	    const char *, const char *subname, DBTYPE, u_int32_t, int);
	virtual int pget(DbTxn *txnid,
	    Dbt *key, Dbt *pkey, Dbt *data, u_int32_t flags);
	virtual int put(DbTxn *, Dbt *, Dbt *, u_int32_t);
	virtual int remove(const char *, const char *, u_int32_t);
	virtual int rename(const char *, const char *, const char *, u_int32_t);
	virtual int set_alloc(
	    db_malloc_fcn_type, db_realloc_fcn_type, db_free_fcn_type);
	virtual void set_app_private(void *);
	virtual int set_append_recno(int (*)(Db *, Dbt *, db_recno_t));
	virtual int set_bt_compare(bt_compare_fcn_type);
	virtual int set_bt_compare(int (*)(Db *, const Dbt *, const Dbt *));
	virtual int set_bt_compress(
	    int (*)
	    (Db *, const Dbt *, const Dbt *, const Dbt *, const Dbt *, Dbt *),
	    int (*)(Db *, const Dbt *, const Dbt *, Dbt *, Dbt *, Dbt *));
	virtual int set_bt_minkey(u_int32_t);
	virtual int set_bt_prefix(bt_prefix_fcn_type);
	virtual int set_bt_prefix(size_t (*)(Db *, const Dbt *, const Dbt *));
	virtual int set_cachesize(u_int32_t, u_int32_t, int);
	virtual int set_create_dir(const char *);
	virtual int set_dup_compare(dup_compare_fcn_type);
	virtual int set_dup_compare(int (*)(Db *, const Dbt *, const Dbt *));
	virtual int set_encrypt(const char *, u_int32_t);
	virtual void set_errcall(
	    void (*)(const DbEnv *, const char *, const char *));
	virtual void set_errfile(FILE *);
	virtual void set_errpfx(const char *);
	virtual int set_feedback(void (*)(Db *, int, int));
	virtual int set_flags(u_int32_t);
	virtual int set_h_compare(h_compare_fcn_type);
	virtual int set_h_compare(int (*)(Db *, const Dbt *, const Dbt *));
	virtual int set_h_ffactor(u_int32_t);
	virtual int set_h_hash(h_hash_fcn_type);
	virtual int set_h_hash(u_int32_t (*)(Db *, const void *, u_int32_t));
	virtual int set_h_nelem(u_int32_t);
	virtual int set_lorder(int);
	virtual void set_msgcall(void (*)(const DbEnv *, const char *));
	virtual void set_msgfile(FILE *);
	virtual int set_pagesize(u_int32_t);
	virtual int set_paniccall(void (*)(DbEnv *, int));
	virtual int set_partition(
	    u_int32_t, Dbt *, u_int32_t (*)(Db *, Dbt *));
	virtual int set_partition_dirs(const char **);
	virtual int set_priority(DB_CACHE_PRIORITY);
	virtual int set_q_extentsize(u_int32_t);
	virtual int set_re_delim(int);
	virtual int set_re_len(u_int32_t);
	virtual int set_re_pad(int);
	virtual int set_re_source(const char *);
	virtual int sort_multiple(Dbt *, Dbt *, u_int32_t);
	virtual int stat(DbTxn *, void *sp, u_int32_t flags);
	virtual int stat_print(u_int32_t flags);
	virtual int sync(u_int32_t flags);
	virtual int truncate(DbTxn *, u_int32_t *, u_int32_t);
	virtual int upgrade(const char *name, u_int32_t flags);
	virtual int verify(
	    const char *, const char *, __DB_STD(ostream) *, u_int32_t);
	virtual void *get_app_private() const;
	virtual __DB_STD(ostream) *get_error_stream();
	virtual void set_error_stream(__DB_STD(ostream) *);
	virtual __DB_STD(ostream) *get_message_stream();
	virtual void set_message_stream(__DB_STD(ostream) *);
	virtual DbEnv *get_env();
	virtual DbMpoolFile *get_mpf();
	virtual ENV *get_ENV()
	{
		return imp_->env;
	}
	virtual DB *get_DB()
	{
		return imp_;
	}
	virtual const DB *get_const_DB() const
	{
		return imp_;
	}
	static Db* get_Db(DB *db)
	{
		return (Db *)db->api_internal;
	}
	static const Db* get_const_Db(const DB *db)
	{
		return (const Db *)db->api_internal;
	}
	u_int32_t get_create_flags() const
	{
		return construct_flags_;
	}
private:
	Db(const Db &);
	Db &operator = (const Db &);
	void cleanup();
	int initialize();
	int error_policy();
	DB *imp_;
	DbEnv *dbenv_;
	DbMpoolFile *mpf_;
	int construct_error_;
	u_int32_t flags_;
	u_int32_t construct_flags_;
public:
	int (*append_recno_callback_)(Db *, Dbt *, db_recno_t);
	int (*associate_callback_)(Db *, const Dbt *, const Dbt *, Dbt *);
	int (*associate_foreign_callback_)
	    (Db *, const Dbt *, Dbt *, const Dbt *, int *);
	int (*bt_compare_callback_)(Db *, const Dbt *, const Dbt *);
	int (*bt_compress_callback_)(
	    Db *, const Dbt *, const Dbt *, const Dbt *, const Dbt *, Dbt *);
	int (*bt_decompress_callback_)(
	    Db *, const Dbt *, const Dbt *, Dbt *, Dbt *, Dbt *);
	size_t (*bt_prefix_callback_)(Db *, const Dbt *, const Dbt *);
	u_int32_t (*db_partition_callback_)(Db *, Dbt *);
	int (*dup_compare_callback_)(Db *, const Dbt *, const Dbt *);
	void (*feedback_callback_)(Db *, int, int);
	int (*h_compare_callback_)(Db *, const Dbt *, const Dbt *);
	u_int32_t (*h_hash_callback_)(Db *, const void *, u_int32_t);
};
class _exported Dbc : protected DBC
{
	friend class Db;
public:
	int close();
	int cmp(Dbc *other_csr, int *result, u_int32_t flags);
	int count(db_recno_t *countp, u_int32_t flags);
	int del(u_int32_t flags);
	int dup(Dbc** cursorp, u_int32_t flags);
	int get(Dbt* key, Dbt *data, u_int32_t flags);
	int get_priority(DB_CACHE_PRIORITY *priorityp);
	int pget(Dbt* key, Dbt* pkey, Dbt *data, u_int32_t flags);
	int put(Dbt* key, Dbt *data, u_int32_t flags);
	int set_priority(DB_CACHE_PRIORITY priority);
private:
	Dbc();
	~Dbc();
	Dbc(const Dbc &);
	Dbc &operator = (const Dbc &);
};
class _exported DbEnv
{
	friend class Db;
	friend class DbLock;
	friend class DbMpoolFile;
public:
	DbEnv(u_int32_t flags);
	virtual ~DbEnv();
	virtual int add_data_dir(const char *);
	virtual int cdsgroup_begin(DbTxn **tid);
	virtual int close(u_int32_t);
	virtual int dbremove(DbTxn *txn, const char *name, const char *subdb,
	    u_int32_t flags);
	virtual int dbrename(DbTxn *txn, const char *name, const char *subdb,
	    const char *newname, u_int32_t flags);
	virtual void err(int, const char *, ...);
	virtual void errx(const char *, ...);
	virtual int failchk(u_int32_t);
	virtual int fileid_reset(const char *, u_int32_t);
	virtual int get_alloc(db_malloc_fcn_type *, db_realloc_fcn_type *,
	    db_free_fcn_type *);
	virtual void *get_app_private() const;
	virtual int get_home(const char **);
	virtual int get_open_flags(u_int32_t *);
	virtual int open(const char *, u_int32_t, int);
	virtual int remove(const char *, u_int32_t);
	virtual int stat_print(u_int32_t flags);
	virtual int set_alloc(db_malloc_fcn_type, db_realloc_fcn_type,
	    db_free_fcn_type);
	virtual void set_app_private(void *);
	virtual int get_cachesize(u_int32_t *, u_int32_t *, int *);
	virtual int set_cachesize(u_int32_t, u_int32_t, int);
	virtual int get_cache_max(u_int32_t *, u_int32_t *);
	virtual int set_cache_max(u_int32_t, u_int32_t);
	virtual int get_create_dir(const char **);
	virtual int set_create_dir(const char *);
	virtual int get_data_dirs(const char ***);
	virtual int set_data_dir(const char *);
	virtual int get_encrypt_flags(u_int32_t *);
	virtual int get_intermediate_dir_mode(const char **);
	virtual int set_intermediate_dir_mode(const char *);
	virtual int get_isalive(
	    int (**)(DbEnv *, pid_t, db_threadid_t, u_int32_t));
	virtual int set_isalive(
	    int (*)(DbEnv *, pid_t, db_threadid_t, u_int32_t));
	virtual int set_encrypt(const char *, u_int32_t);
	virtual void get_errcall(
	    void (**)(const DbEnv *, const char *, const char *));
	virtual void set_errcall(
	    void (*)(const DbEnv *, const char *, const char *));
	virtual void get_errfile(FILE **);
	virtual void set_errfile(FILE *);
	virtual void get_errpfx(const char **);
	virtual void set_errpfx(const char *);
	virtual int set_event_notify(void (*)(DbEnv *, u_int32_t, void *));
	virtual int get_flags(u_int32_t *);
	virtual int set_flags(u_int32_t, int);
	virtual bool is_bigendian();
	virtual int lsn_reset(const char *, u_int32_t);
	virtual int get_feedback(void (**)(DbEnv *, int, int));
	virtual int set_feedback(void (*)(DbEnv *, int, int));
	virtual int get_lg_bsize(u_int32_t *);
	virtual int set_lg_bsize(u_int32_t);
	virtual int get_lg_dir(const char **);
	virtual int set_lg_dir(const char *);
	virtual int get_lg_filemode(int *);
	virtual int set_lg_filemode(int);
	virtual int get_lg_max(u_int32_t *);
	virtual int set_lg_max(u_int32_t);
	virtual int get_lg_regionmax(u_int32_t *);
	virtual int set_lg_regionmax(u_int32_t);
	virtual int get_lk_conflicts(const u_int8_t **, int *);
	virtual int set_lk_conflicts(u_int8_t *, int);
	virtual int get_lk_detect(u_int32_t *);
	virtual int set_lk_detect(u_int32_t);
	virtual int get_lk_max_lockers(u_int32_t *);
	virtual int set_lk_max_lockers(u_int32_t);
	virtual int get_lk_max_locks(u_int32_t *);
	virtual int set_lk_max_locks(u_int32_t);
	virtual int get_lk_max_objects(u_int32_t *);
	virtual int set_lk_max_objects(u_int32_t);
	virtual int get_lk_partitions(u_int32_t *);
	virtual int set_lk_partitions(u_int32_t);
	virtual int get_mp_mmapsize(size_t *);
	virtual int set_mp_mmapsize(size_t);
	virtual int get_mp_max_openfd(int *);
	virtual int set_mp_max_openfd(int);
	virtual int get_mp_max_write(int *, db_timeout_t *);
	virtual int set_mp_max_write(int, db_timeout_t);
	virtual int get_mp_pagesize(u_int32_t *);
	virtual int set_mp_pagesize(u_int32_t);
	virtual int get_mp_tablesize(u_int32_t *);
	virtual int set_mp_tablesize(u_int32_t);
	virtual void get_msgcall(void (**)(const DbEnv *, const char *));
	virtual void set_msgcall(void (*)(const DbEnv *, const char *));
	virtual void get_msgfile(FILE **);
	virtual void set_msgfile(FILE *);
	virtual int set_paniccall(void (*)(DbEnv *, int));
	virtual int set_rpc_server(void *, char *, long, long, u_int32_t);
	virtual int get_shm_key(long *);
	virtual int set_shm_key(long);
	virtual int get_timeout(db_timeout_t *, u_int32_t);
	virtual int set_timeout(db_timeout_t, u_int32_t);
	virtual int get_tmp_dir(const char **);
	virtual int set_tmp_dir(const char *);
	virtual int get_tx_max(u_int32_t *);
	virtual int set_tx_max(u_int32_t);
	virtual int get_app_dispatch(
	    int (**)(DbEnv *, Dbt *, DbLsn *, db_recops));
	virtual int set_app_dispatch(int (*)(DbEnv *,
	    Dbt *, DbLsn *, db_recops));
	virtual int get_tx_timestamp(time_t *);
	virtual int set_tx_timestamp(time_t *);
	virtual int get_verbose(u_int32_t which, int *);
	virtual int set_verbose(u_int32_t which, int);
	static char *version(int *major, int *minor, int *patch);
	static char *strerror(int);
	virtual __DB_STD(ostream) *get_error_stream();
	virtual void set_error_stream(__DB_STD(ostream) *);
	virtual __DB_STD(ostream) *get_message_stream();
	virtual void set_message_stream(__DB_STD(ostream) *);
	static void runtime_error(DbEnv *dbenv, const char *caller, int err,
				  int error_policy);
	static void runtime_error_dbt(DbEnv *dbenv, const char *caller, Dbt *dbt,
				  int error_policy);
	static void runtime_error_lock_get(DbEnv *dbenv, const char *caller,
				  int err, db_lockop_t op, db_lockmode_t mode,
				  Dbt *obj, DbLock lock, int index,
				  int error_policy);
	virtual int lock_detect(u_int32_t flags, u_int32_t atype, int *aborted);
	virtual int lock_get(u_int32_t locker, u_int32_t flags, Dbt *obj,
		     db_lockmode_t lock_mode, DbLock *lock);
	virtual int lock_id(u_int32_t *idp);
	virtual int lock_id_free(u_int32_t id);
	virtual int lock_put(DbLock *lock);
	virtual int lock_stat(DB_LOCK_STAT **statp, u_int32_t flags);
	virtual int lock_stat_print(u_int32_t flags);
	virtual int lock_vec(u_int32_t locker, u_int32_t flags,
		     DB_LOCKREQ list[], int nlist, DB_LOCKREQ **elistp);
	virtual int log_archive(char **list[], u_int32_t flags);
	static int log_compare(const DbLsn *lsn0, const DbLsn *lsn1);
	virtual int log_cursor(DbLogc **cursorp, u_int32_t flags);
	virtual int log_file(DbLsn *lsn, char *namep, size_t len);
	virtual int log_flush(const DbLsn *lsn);
	virtual int log_get_config(u_int32_t, int *);
	virtual int log_put(DbLsn *lsn, const Dbt *data, u_int32_t flags);
	virtual int log_printf(DbTxn *, const char *, ...);
	virtual int log_set_config(u_int32_t, int);
	virtual int log_stat(DB_LOG_STAT **spp, u_int32_t flags);
	virtual int log_stat_print(u_int32_t flags);
	virtual int memp_fcreate(DbMpoolFile **dbmfp, u_int32_t flags);
	virtual int memp_register(int ftype,
			  pgin_fcn_type pgin_fcn,
			  pgout_fcn_type pgout_fcn);
	virtual int memp_stat(DB_MPOOL_STAT
		      **gsp, DB_MPOOL_FSTAT ***fsp, u_int32_t flags);
	virtual int memp_stat_print(u_int32_t flags);
	virtual int memp_sync(DbLsn *lsn);
	virtual int memp_trickle(int pct, int *nwrotep);
	virtual int mutex_alloc(u_int32_t, db_mutex_t *);
	virtual int mutex_free(db_mutex_t);
	virtual int mutex_get_align(u_int32_t *);
	virtual int mutex_get_increment(u_int32_t *);
	virtual int mutex_get_max(u_int32_t *);
	virtual int mutex_get_tas_spins(u_int32_t *);
	virtual int mutex_lock(db_mutex_t);
	virtual int mutex_set_align(u_int32_t);
	virtual int mutex_set_increment(u_int32_t);
	virtual int mutex_set_max(u_int32_t);
	virtual int mutex_set_tas_spins(u_int32_t);
	virtual int mutex_stat(DB_MUTEX_STAT **, u_int32_t);
	virtual int mutex_stat_print(u_int32_t);
	virtual int mutex_unlock(db_mutex_t);
	virtual int txn_begin(DbTxn *pid, DbTxn **tid, u_int32_t flags);
	virtual int txn_checkpoint(u_int32_t kbyte, u_int32_t min,
			u_int32_t flags);
	virtual int txn_recover(DbPreplist *preplist, u_int32_t count,
			u_int32_t *retp, u_int32_t flags);
	virtual int txn_stat(DB_TXN_STAT **statp, u_int32_t flags);
	virtual int txn_stat_print(u_int32_t flags);
	virtual int rep_elect(u_int32_t, u_int32_t, u_int32_t);
	virtual int rep_flush();
	virtual int rep_process_message(Dbt *, Dbt *, int, DbLsn *);
	virtual int rep_start(Dbt *, u_int32_t);
	virtual int rep_stat(DB_REP_STAT **statp, u_int32_t flags);
	virtual int rep_stat_print(u_int32_t flags);
	virtual int rep_get_clockskew(u_int32_t *, u_int32_t *);
	virtual int rep_set_clockskew(u_int32_t, u_int32_t);
	virtual int rep_get_limit(u_int32_t *, u_int32_t *);
	virtual int rep_set_limit(u_int32_t, u_int32_t);
	virtual int rep_set_transport(int, int (*)(DbEnv *,
	    const Dbt *, const Dbt *, const DbLsn *, int, u_int32_t));
	virtual int rep_set_request(u_int32_t, u_int32_t);
	virtual int rep_get_request(u_int32_t *, u_int32_t *);
	virtual int get_thread_count(u_int32_t *);
	virtual int set_thread_count(u_int32_t);
	virtual int get_thread_id_fn(
	    void (**)(DbEnv *, pid_t *, db_threadid_t *));
	virtual int set_thread_id(void (*)(DbEnv *, pid_t *, db_threadid_t *));
	virtual int get_thread_id_string_fn(
	    char *(**)(DbEnv *, pid_t, db_threadid_t, char *));
	virtual int set_thread_id_string(char *(*)(DbEnv *,
	    pid_t, db_threadid_t, char *));
	virtual int rep_set_config(u_int32_t, int);
	virtual int rep_get_config(u_int32_t, int *);
	virtual int rep_sync(u_int32_t flags);
	virtual int rep_get_nsites(u_int32_t *n);
	virtual int rep_set_nsites(u_int32_t n);
	virtual int rep_get_priority(u_int32_t *priorityp);
	virtual int rep_set_priority(u_int32_t priority);
	virtual int rep_get_timeout(int which, db_timeout_t *timeout);
	virtual int rep_set_timeout(int which, db_timeout_t timeout);
	virtual int repmgr_add_remote_site(const char * host, u_int16_t port,
	    int *eidp, u_int32_t flags);
	virtual int repmgr_get_ack_policy(int *policy);
	virtual int repmgr_set_ack_policy(int policy);
	virtual int repmgr_set_local_site(const char * host, u_int16_t port,
	    u_int32_t flags);
	virtual int repmgr_site_list(u_int *countp, DB_REPMGR_SITE **listp);
	virtual int repmgr_start(int nthreads, u_int32_t flags);
	virtual int repmgr_stat(DB_REPMGR_STAT **statp, u_int32_t flags);
	virtual int repmgr_stat_print(u_int32_t flags);
	virtual ENV *get_ENV()
	{
		return imp_->env;
	}
	virtual DB_ENV *get_DB_ENV()
	{
		return imp_;
	}
	virtual const DB_ENV *get_const_DB_ENV() const
	{
		return imp_;
	}
	static DbEnv* get_DbEnv(DB_ENV *dbenv)
	{
		return dbenv ? (DbEnv *)dbenv->api1_internal : 0;
	}
	static const DbEnv* get_const_DbEnv(const DB_ENV *dbenv)
	{
		return dbenv ? (const DbEnv *)dbenv->api1_internal : 0;
	}
	u_int32_t get_create_flags() const
	{
		return construct_flags_;
	}
	static DbEnv* wrap_DB_ENV(DB_ENV *dbenv);
	static int _app_dispatch_intercept(DB_ENV *dbenv, DBT *dbt, DB_LSN *lsn,
				       db_recops op);
	static void _paniccall_intercept(DB_ENV *dbenv, int errval);
	static void _feedback_intercept(DB_ENV *dbenv, int opcode, int pct);
	static void  _event_func_intercept(DB_ENV *dbenv, u_int32_t, void *);
	static int _isalive_intercept(DB_ENV *dbenv, pid_t pid,
	    db_threadid_t thrid, u_int32_t flags);
	static int _rep_send_intercept(DB_ENV *dbenv, const DBT *cntrl,
	    const DBT *data, const DB_LSN *lsn, int id, u_int32_t flags);
	static void _stream_error_function(const DB_ENV *dbenv,
	    const char *prefix, const char *message);
	static void _stream_message_function(const DB_ENV *dbenv,
	    const char *message);
	static void _thread_id_intercept(DB_ENV *dbenv, pid_t *pidp,
	    db_threadid_t *thridp);
	static char *_thread_id_string_intercept(DB_ENV *dbenv, pid_t pid,
	    db_threadid_t thrid, char *buf);
private:
	void cleanup();
	int initialize(DB_ENV *dbenv);
	int error_policy();
	DbEnv(DB_ENV *, u_int32_t flags);
	DbEnv(const DbEnv &);
	void operator = (const DbEnv &);
	DB_ENV *imp_;
	int construct_error_;
	u_int32_t construct_flags_;
	__DB_STD(ostream) *error_stream_;
	__DB_STD(ostream) *message_stream_;
	int (*app_dispatch_callback_)(DbEnv *, Dbt *, DbLsn *, db_recops);
	int (*isalive_callback_)(DbEnv *, pid_t, db_threadid_t, u_int32_t);
	void (*error_callback_)(const DbEnv *, const char *, const char *);
	void (*feedback_callback_)(DbEnv *, int, int);
	void (*message_callback_)(const DbEnv *, const char *);
	void (*paniccall_callback_)(DbEnv *, int);
	void (*event_func_callback_)(DbEnv *, u_int32_t, void *);
	int (*rep_send_callback_)(DbEnv *, const Dbt *, const Dbt *,
	    const DbLsn *, int, u_int32_t);
	void (*thread_id_callback_)(DbEnv *, pid_t *, db_threadid_t *);
	char *(*thread_id_string_callback_)(DbEnv *, pid_t, db_threadid_t,
	    char *);
};
class _exported DbLock
{
	friend class DbEnv;
public:
	DbLock();
	DbLock(const DbLock &);
	DbLock &operator = (const DbLock &);
protected:
	DbLock(DB_LOCK);
	DB_LOCK lock_;
};
class _exported DbLogc : protected DB_LOGC
{
	friend class DbEnv;
public:
	int close(u_int32_t _flags);
	int get(DbLsn *lsn, Dbt *data, u_int32_t _flags);
	int version(u_int32_t *versionp, u_int32_t _flags);
private:
	DbLogc();
	~DbLogc();
	DbLogc(const Dbc &);
	DbLogc &operator = (const Dbc &);
};
class _exported DbLsn : public DB_LSN
{
	friend class DbEnv;
	friend class DbLogc;
};
class _exported DbMpoolFile
{
	friend class DbEnv;
	friend class Db;
public:
	int close(u_int32_t flags);
	int get(db_pgno_t *pgnoaddr, DbTxn *txn, u_int32_t flags, void *pagep);
	int get_clear_len(u_int32_t *len);
	int get_fileid(u_int8_t *fileid);
	int get_flags(u_int32_t *flagsp);
	int get_ftype(int *ftype);
	int get_last_pgno(db_pgno_t *pgnop);
	int get_lsn_offset(int32_t *offsetp);
	int get_maxsize(u_int32_t *gbytes, u_int32_t *bytes);
	int get_pgcookie(DBT *dbt);
	int get_priority(DB_CACHE_PRIORITY *priorityp);
	int get_transactional(void);
	int open(const char *file, u_int32_t flags, int mode, size_t pagesize);
	int put(void *pgaddr, DB_CACHE_PRIORITY priority, u_int32_t flags);
	int set_clear_len(u_int32_t len);
	int set_fileid(u_int8_t *fileid);
	int set_flags(u_int32_t flags, int onoff);
	int set_ftype(int ftype);
	int set_lsn_offset(int32_t offset);
	int set_maxsize(u_int32_t gbytes, u_int32_t bytes);
	int set_pgcookie(DBT *dbt);
	int set_priority(DB_CACHE_PRIORITY priority);
	int sync();
	virtual DB_MPOOLFILE *get_DB_MPOOLFILE()
	{
		return imp_;
	}
	virtual const DB_MPOOLFILE *get_const_DB_MPOOLFILE() const
	{
		return imp_;
	}
private:
	DB_MPOOLFILE *imp_;
	DbMpoolFile();
protected:
	virtual ~DbMpoolFile();
private:
	DbMpoolFile(const DbMpoolFile &);
	void operator = (const DbMpoolFile &);
};
class _exported DbPreplist
{
public:
	DbTxn *txn;
	u_int8_t gid[DB_GID_SIZE];
};
class _exported DbSequence
{
public:
	DbSequence(Db *db, u_int32_t flags);
	virtual ~DbSequence();
	int open(DbTxn *txnid, Dbt *key, u_int32_t flags);
	int initial_value(db_seq_t value);
	int close(u_int32_t flags);
	int remove(DbTxn *txnid, u_int32_t flags);
	int stat(DB_SEQUENCE_STAT **sp, u_int32_t flags);
	int stat_print(u_int32_t flags);
	int get(DbTxn *txnid, int32_t delta, db_seq_t *retp, u_int32_t flags);
	int get_cachesize(int32_t *sizep);
	int set_cachesize(int32_t size);
	int get_flags(u_int32_t *flagsp);
	int set_flags(u_int32_t flags);
	int get_range(db_seq_t *minp, db_seq_t *maxp);
	int set_range(db_seq_t min, db_seq_t max);
	Db *get_db();
	Dbt *get_key();
	virtual DB_SEQUENCE *get_DB_SEQUENCE()
	{
		return imp_;
	}
	virtual const DB_SEQUENCE *get_const_DB_SEQUENCE() const
	{
		return imp_;
	}
	static DbSequence* get_DbSequence(DB_SEQUENCE *seq)
	{
		return (DbSequence *)seq->api_internal;
	}
	static const DbSequence* get_const_DbSequence(const DB_SEQUENCE *seq)
	{
		return (const DbSequence *)seq->api_internal;
	}
	static DbSequence* wrap_DB_SEQUENCE(DB_SEQUENCE *seq);
private:
	DbSequence(DB_SEQUENCE *seq);
	DbSequence(const DbSequence &);
	DbSequence &operator = (const DbSequence &);
	DB_SEQUENCE *imp_;
	DBT key_;
};
class _exported DbTxn
{
	friend class DbEnv;
public:
	int abort();
	int commit(u_int32_t flags);
	int discard(u_int32_t flags);
	u_int32_t id();
	int get_name(const char **namep);
	int prepare(u_int8_t *gid);
	int set_name(const char *name);
	int set_timeout(db_timeout_t timeout, u_int32_t flags);
	virtual DB_TXN *get_DB_TXN()
	{
		return imp_;
	}
	virtual const DB_TXN *get_const_DB_TXN() const
	{
		return imp_;
	}
	static DbTxn* get_DbTxn(DB_TXN *txn)
	{
		return (DbTxn *)txn->api_internal;
	}
	static const DbTxn* get_const_DbTxn(const DB_TXN *txn)
	{
		return (const DbTxn *)txn->api_internal;
	}
	static DbTxn* wrap_DB_TXN(DB_TXN *txn);
	void remove_child_txn(DbTxn *kid);
	void add_child_txn(DbTxn *kid);
	void set_parent(DbTxn *ptxn)
	{
		parent_txn_ = ptxn;
	}
private:
	DB_TXN *imp_;
	DbTxn *parent_txn_;
	DbTxn(DbTxn *ptxn);
	DbTxn(DB_TXN *txn, DbTxn *ptxn);
	virtual ~DbTxn();
	DbTxn(const DbTxn &);
	void operator = (const DbTxn &);
	struct __children {
		DbTxn *tqh_first;
		DbTxn **tqh_last;
	} children;
	struct {
		DbTxn *tqe_next;
		DbTxn **tqe_prev;
	} child_entry;
};
class _exported Dbt : private DBT
{
	friend class Db;
	friend class Dbc;
	friend class DbEnv;
	friend class DbLogc;
	friend class DbSequence;
public:
	void *get_data() const                 { return data; }
	void set_data(void *value)             { data = value; }
	u_int32_t get_size() const             { return size; }
	void set_size(u_int32_t value)         { size = value; }
	u_int32_t get_ulen() const             { return ulen; }
	void set_ulen(u_int32_t value)         { ulen = value; }
	u_int32_t get_dlen() const             { return dlen; }
	void set_dlen(u_int32_t value)         { dlen = value; }
	u_int32_t get_doff() const             { return doff; }
	void set_doff(u_int32_t value)         { doff = value; }
	u_int32_t get_flags() const            { return flags; }
	void set_flags(u_int32_t value)        { flags = value; }
	DBT *get_DBT()                         { return (DBT *)this; }
	const DBT *get_const_DBT() const       { return (const DBT *)this; }
	static Dbt* get_Dbt(DBT *dbt)          { return (Dbt *)dbt; }
	static const Dbt* get_const_Dbt(const DBT *dbt)
					       { return (const Dbt *)dbt; }
	Dbt(void *data, u_int32_t size);
	Dbt();
	~Dbt();
	Dbt(const Dbt &);
	Dbt &operator = (const Dbt &);
private:
};
class _exported DbMultipleIterator
{
public:
	DbMultipleIterator(const Dbt &dbt);
protected:
	u_int8_t *data_;
	u_int32_t *p_;
};
class _exported DbMultipleKeyDataIterator : private DbMultipleIterator
{
public:
	DbMultipleKeyDataIterator(const Dbt &dbt) : DbMultipleIterator(dbt) {}
	bool next(Dbt &key, Dbt &data);
};
class _exported DbMultipleRecnoDataIterator : private DbMultipleIterator
{
public:
	DbMultipleRecnoDataIterator(const Dbt &dbt) : DbMultipleIterator(dbt) {}
	bool next(db_recno_t &recno, Dbt &data);
};
class _exported DbMultipleDataIterator : private DbMultipleIterator
{
public:
	DbMultipleDataIterator(const Dbt &dbt) : DbMultipleIterator(dbt) {}
	bool next(Dbt &data);
};
class _exported DbMultipleBuilder
{
public:
	DbMultipleBuilder(Dbt &dbt);
protected:
	Dbt &dbt_;
	void *p_;
};
class _exported DbMultipleDataBuilder : DbMultipleBuilder
{
public:
	DbMultipleDataBuilder(Dbt &dbt) : DbMultipleBuilder(dbt) {}
	bool append(void *dbuf, size_t dlen);
	bool reserve(void *&ddest, size_t dlen);
};
class _exported DbMultipleKeyDataBuilder : DbMultipleBuilder
{
public:
	DbMultipleKeyDataBuilder(Dbt &dbt) : DbMultipleBuilder(dbt) {}
	bool append(void *kbuf, size_t klen, void *dbuf, size_t dlen);
	bool reserve(void *&kdest, size_t klen, void *&ddest, size_t dlen);
};
class _exported DbMultipleRecnoDataBuilder
{
public:
	DbMultipleRecnoDataBuilder(Dbt &dbt);
	bool append(db_recno_t recno, void *dbuf, size_t dlen);
	bool reserve(db_recno_t recno, void *&ddest, size_t dlen);
protected:
	Dbt &dbt_;
	void *p_;
};
class _exported DbException : public __DB_STD(exception)
{
public:
	virtual ~DbException() throw();
	DbException(int err);
	DbException(const char *description);
	DbException(const char *description, int err);
	DbException(const char *prefix, const char *description, int err);
	int get_errno() const;
	virtual const char *what() const throw();
	DbEnv *get_env() const;
	void set_env(DbEnv *dbenv);
	DbException(const DbException &);
	DbException &operator = (const DbException &);
private:
	void describe(const char *prefix, const char *description);
	char *what_;
	int err_;
	DbEnv *dbenv_;
};
class _exported DbDeadlockException : public DbException
{
public:
	virtual ~DbDeadlockException() throw();
	DbDeadlockException(const char *description);
	DbDeadlockException(const DbDeadlockException &);
	DbDeadlockException &operator = (const DbDeadlockException &);
};
class _exported DbLockNotGrantedException : public DbException
{
public:
	virtual ~DbLockNotGrantedException() throw();
	DbLockNotGrantedException(const char *prefix, db_lockop_t op,
	    db_lockmode_t mode, const Dbt *obj, const DbLock lock, int index);
	DbLockNotGrantedException(const char *description);
	DbLockNotGrantedException(const DbLockNotGrantedException &);
	DbLockNotGrantedException &operator =
	    (const DbLockNotGrantedException &);
	db_lockop_t get_op() const;
	db_lockmode_t get_mode() const;
	const Dbt* get_obj() const;
	DbLock *get_lock() const;
	int get_index() const;
private:
	db_lockop_t op_;
	db_lockmode_t mode_;
	const Dbt *obj_;
	DbLock *lock_;
	int index_;
};
class _exported DbMemoryException : public DbException
{
public:
	virtual ~DbMemoryException() throw();
	DbMemoryException(Dbt *dbt);
	DbMemoryException(const char *prefix, Dbt *dbt);
	DbMemoryException(const DbMemoryException &);
	DbMemoryException &operator = (const DbMemoryException &);
	Dbt *get_dbt() const;
private:
	Dbt *dbt_;
};
class _exported DbRepHandleDeadException : public DbException
{
public:
	virtual ~DbRepHandleDeadException() throw();
	DbRepHandleDeadException(const char *description);
	DbRepHandleDeadException(const DbRepHandleDeadException &);
	DbRepHandleDeadException &operator = (const DbRepHandleDeadException &);
};
class _exported DbRunRecoveryException : public DbException
{
public:
	virtual ~DbRunRecoveryException() throw();
	DbRunRecoveryException(const char *description);
	DbRunRecoveryException(const DbRunRecoveryException &);
	DbRunRecoveryException &operator = (const DbRunRecoveryException &);
};
#ifdef _MSC_VER
#pragma warning(pop)
#endif
#endif
