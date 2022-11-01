#ifndef EVENT2_HTTP_COMPAT_H_INCLUDED_
#define EVENT2_HTTP_COMPAT_H_INCLUDED_
#ifdef __cplusplus
extern "C" {
#endif
#include <event2/event-config.h>
#ifdef EVENT__HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef EVENT__HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#include <event2/util.h>
EVENT2_EXPORT_SYMBOL
struct evhttp *evhttp_start(const char *address, ev_uint16_t port);
EVENT2_EXPORT_SYMBOL
struct evhttp_connection *evhttp_connection_new(
	const char *address, ev_uint16_t port);
EVENT2_EXPORT_SYMBOL
void evhttp_connection_set_base(struct evhttp_connection *evcon,
    struct event_base *base);
#define evhttp_request_uri evhttp_request_get_uri
#ifdef __cplusplus
}
#endif
#endif