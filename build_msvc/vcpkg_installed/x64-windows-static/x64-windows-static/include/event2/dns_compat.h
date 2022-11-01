#ifndef EVENT2_DNS_COMPAT_H_INCLUDED_
#define EVENT2_DNS_COMPAT_H_INCLUDED_
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
#include <event2/visibility.h>
EVENT2_EXPORT_SYMBOL
int evdns_init(void);
struct evdns_base;
EVENT2_EXPORT_SYMBOL
struct evdns_base *evdns_get_global_base(void);
EVENT2_EXPORT_SYMBOL
void evdns_shutdown(int fail_requests);
EVENT2_EXPORT_SYMBOL
int evdns_nameserver_add(unsigned long int address);
EVENT2_EXPORT_SYMBOL
int evdns_count_nameservers(void);
EVENT2_EXPORT_SYMBOL
int evdns_clear_nameservers_and_suspend(void);
EVENT2_EXPORT_SYMBOL
int evdns_resume(void);
EVENT2_EXPORT_SYMBOL
int evdns_nameserver_ip_add(const char *ip_as_string);
EVENT2_EXPORT_SYMBOL
int evdns_resolve_ipv4(const char *name, int flags, evdns_callback_type callback, void *ptr);
EVENT2_EXPORT_SYMBOL
int evdns_resolve_ipv6(const char *name, int flags, evdns_callback_type callback, void *ptr);
struct in_addr;
struct in6_addr;
EVENT2_EXPORT_SYMBOL
int evdns_resolve_reverse(const struct in_addr *in, int flags, evdns_callback_type callback, void *ptr);
EVENT2_EXPORT_SYMBOL
int evdns_resolve_reverse_ipv6(const struct in6_addr *in, int flags, evdns_callback_type callback, void *ptr);
EVENT2_EXPORT_SYMBOL
int evdns_set_option(const char *option, const char *val, int flags);
EVENT2_EXPORT_SYMBOL
int evdns_resolv_conf_parse(int flags, const char *const filename);
EVENT2_EXPORT_SYMBOL
void evdns_search_clear(void);
EVENT2_EXPORT_SYMBOL
void evdns_search_add(const char *domain);
EVENT2_EXPORT_SYMBOL
void evdns_search_ndots_set(const int ndots);
EVENT2_EXPORT_SYMBOL
struct evdns_server_port *
evdns_add_server_port(evutil_socket_t socket, int flags,
	evdns_request_callback_fn_type callback, void *user_data);
#ifdef _WIN32
EVENT2_EXPORT_SYMBOL
int evdns_config_windows_nameservers(void);
#define EVDNS_CONFIG_WINDOWS_NAMESERVERS_IMPLEMENTED
#endif
#ifdef __cplusplus
}
#endif
#endif
