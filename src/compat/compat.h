#ifndef LCRYP_COMPAT_COMPAT_H
#define LCRYP_COMPAT_COMPAT_H
#if defined(HAVE_CONFIG_H)
#include <config/lcryp-config.h>
#endif
#ifdef WIN32
#ifdef FD_SETSIZE
#undef FD_SETSIZE
#endif
#define FD_SETSIZE 1024
#include <winsock2.h>
#include <ws2tcpip.h>
#include <cstdint>
#else
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <limits.h>
#include <netdb.h>
#include <unistd.h>
#endif
#ifndef WIN32
typedef unsigned int SOCKET;
#include <cerrno>
#define WSAGetLastError()   errno
#define WSAEINVAL           EINVAL
#define WSAEWOULDBLOCK      EWOULDBLOCK
#define WSAEAGAIN           EAGAIN
#define WSAEMSGSIZE         EMSGSIZE
#define WSAEINTR            EINTR
#define WSAEINPROGRESS      EINPROGRESS
#define WSAEADDRINUSE       EADDRINUSE
#define INVALID_SOCKET      (SOCKET)(~0)
#define SOCKET_ERROR        -1
#else
#ifdef EAGAIN
#define WSAEAGAIN EAGAIN
#else
#define WSAEAGAIN WSAEWOULDBLOCK
#endif
#endif
#ifdef WIN32
#ifndef S_IRUSR
#define S_IRUSR             0400
#define S_IWUSR             0200
#endif
#endif
#ifndef WIN32
#define MAX_PATH            1024
#endif
#ifdef _MSC_VER
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
#endif
#ifndef WIN32
typedef void* sockopt_arg_type;
#else
typedef char* sockopt_arg_type;
#endif
#ifdef WIN32
#define MAIN_FUNCTION __declspec(dllexport) int main(int argc, char* argv[])
#else
#define MAIN_FUNCTION int main(int argc, char* argv[])
#endif
#if defined(__linux__)
#define USE_POLL
#endif
bool static inline IsSelectableSocket(const SOCKET& s) {
#if defined(USE_POLL) || defined(WIN32)
    return true;
#else
    return (s < FD_SETSIZE);
#endif
}
#if !defined(MSG_NOSIGNAL)
#define MSG_NOSIGNAL 0
#endif
#if !defined(MSG_DONTWAIT)
#define MSG_DONTWAIT 0
#endif
#endif
