#ifndef LCRYP_NETBASE_H
#define LCRYP_NETBASE_H
#if defined(HAVE_CONFIG_H)
#include <config/lcryp-config.h>
#endif
#include <compat/compat.h>
#include <netaddress.h>
#include <serialize.h>
#include <util/sock.h>
#include <functional>
#include <memory>
#include <stdint.h>
#include <string>
#include <type_traits>
#include <vector>
extern int nConnectTimeout;
extern bool fNameLookup;
static const int DEFAULT_CONNECT_TIMEOUT = 5000;
static const int DEFAULT_NAME_LOOKUP = true;
enum class ConnectionDirection {
    None = 0,
    In = (1U << 0),
    Out = (1U << 1),
    Both = (In | Out),
};
static inline ConnectionDirection& operator|=(ConnectionDirection& a, ConnectionDirection b) {
    using underlying = typename std::underlying_type<ConnectionDirection>::type;
    a = ConnectionDirection(underlying(a) | underlying(b));
    return a;
}
static inline bool operator&(ConnectionDirection a, ConnectionDirection b) {
    using underlying = typename std::underlying_type<ConnectionDirection>::type;
    return (underlying(a) & underlying(b));
}
class Proxy
{
public:
    Proxy(): randomize_credentials(false) {}
    explicit Proxy(const CService &_proxy, bool _randomize_credentials=false): proxy(_proxy), randomize_credentials(_randomize_credentials) {}
    bool IsValid() const { return proxy.IsValid(); }
    CService proxy;
    bool randomize_credentials;
};
struct ProxyCredentials
{
    std::string username;
    std::string password;
};
std::vector<CNetAddr> WrappedGetAddrInfo(const std::string& name, bool allow_lookup);
enum Network ParseNetwork(const std::string& net);
std::string GetNetworkName(enum Network net);
std::vector<std::string> GetNetworkNames(bool append_unroutable = false);
bool SetProxy(enum Network net, const Proxy &addrProxy);
bool GetProxy(enum Network net, Proxy &proxyInfoOut);
bool IsProxy(const CNetAddr &addr);
bool SetNameProxy(const Proxy &addrProxy);
bool HaveNameProxy();
bool GetNameProxy(Proxy &nameProxyOut);
using DNSLookupFn = std::function<std::vector<CNetAddr>(const std::string&, bool)>;
extern DNSLookupFn g_dns_lookup;
bool LookupHost(const std::string& name, std::vector<CNetAddr>& vIP, unsigned int nMaxSolutions, bool fAllowLookup, DNSLookupFn dns_lookup_function = g_dns_lookup);
bool LookupHost(const std::string& name, CNetAddr& addr, bool fAllowLookup, DNSLookupFn dns_lookup_function = g_dns_lookup);
bool Lookup(const std::string& name, std::vector<CService>& vAddr, uint16_t portDefault, bool fAllowLookup, unsigned int nMaxSolutions, DNSLookupFn dns_lookup_function = g_dns_lookup);
bool Lookup(const std::string& name, CService& addr, uint16_t portDefault, bool fAllowLookup, DNSLookupFn dns_lookup_function = g_dns_lookup);
CService LookupNumeric(const std::string& name, uint16_t portDefault = 0, DNSLookupFn dns_lookup_function = g_dns_lookup);
bool LookupSubNet(const std::string& subnet_str, CSubNet& subnet_out);
std::unique_ptr<Sock> CreateSockTCP(const CService& address_family);
extern std::function<std::unique_ptr<Sock>(const CService&)> CreateSock;
bool ConnectSocketDirectly(const CService &addrConnect, const Sock& sock, int nTimeout, bool manual_connection);
bool ConnectThroughProxy(const Proxy& proxy, const std::string& strDest, uint16_t port, const Sock& sock, int nTimeout, bool& outProxyConnectionFailed);
bool SetSocketNonBlocking(const SOCKET& hSocket);
void InterruptSocks5(bool interrupt);
bool Socks5(const std::string& strDest, uint16_t port, const ProxyCredentials* auth, const Sock& socket);
bool IsBadPort(uint16_t port);
#endif
