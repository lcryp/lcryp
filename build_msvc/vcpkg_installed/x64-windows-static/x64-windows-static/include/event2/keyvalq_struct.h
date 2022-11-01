#ifndef EVENT2_KEYVALQ_STRUCT_H_INCLUDED_
#define EVENT2_KEYVALQ_STRUCT_H_INCLUDED_
#ifdef __cplusplus
extern "C" {
#endif
#ifndef TAILQ_ENTRY
#define EVENT_DEFINED_TQENTRY_
#define TAILQ_ENTRY(type)						\
struct {								\
	struct type *tqe_next;	 			\
	struct type **tqe_prev;	 	\
}
#endif
#ifndef TAILQ_HEAD
#define EVENT_DEFINED_TQHEAD_
#define TAILQ_HEAD(name, type)			\
struct name {					\
	struct type *tqh_first;			\
	struct type **tqh_last;			\
}
#endif
struct evkeyval {
	TAILQ_ENTRY(evkeyval) next;
	char *key;
	char *value;
};
TAILQ_HEAD (evkeyvalq, evkeyval);
#ifdef EVENT_DEFINED_TQENTRY_
#undef TAILQ_ENTRY
#endif
#ifdef EVENT_DEFINED_TQHEAD_
#undef TAILQ_HEAD
#endif
#ifdef __cplusplus
}
#endif
#endif