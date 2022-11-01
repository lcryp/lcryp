#ifndef LCRYP_TORCONTROL_H
#define LCRYP_TORCONTROL_H
#include <fs.h>
#include <netaddress.h>
#include <boost/signals2/signal.hpp>
#include <event2/bufferevent.h>
#include <event2/event.h>
#include <cstdlib>
#include <deque>
#include <functional>
#include <string>
#include <vector>
class CService;
extern const std::string DEFAULT_TOR_CONTROL;
static const bool DEFAULT_LISTEN_ONION = true;
void StartTorControl(CService onion_service_target);
void InterruptTorControl();
void StopTorControl();
CService DefaultOnionServiceTarget();
class TorControlReply
{
public:
    TorControlReply() { Clear(); }
    int code;
    std::vector<std::string> lines;
    void Clear()
    {
        code = 0;
        lines.clear();
    }
};
class TorControlConnection
{
public:
    typedef std::function<void(TorControlConnection&)> ConnectionCB;
    typedef std::function<void(TorControlConnection &,const TorControlReply &)> ReplyHandlerCB;
    explicit TorControlConnection(struct event_base *base);
    ~TorControlConnection();
    bool Connect(const std::string& tor_control_center, const ConnectionCB& connected, const ConnectionCB& disconnected);
    void Disconnect();
    bool Command(const std::string &cmd, const ReplyHandlerCB& reply_handler);
    boost::signals2::signal<void(TorControlConnection &,const TorControlReply &)> async_handler;
private:
    std::function<void(TorControlConnection&)> connected;
    std::function<void(TorControlConnection&)> disconnected;
    struct event_base *base;
    struct bufferevent *b_conn;
    TorControlReply message;
    std::deque<ReplyHandlerCB> reply_handlers;
    static void readcb(struct bufferevent *bev, void *ctx);
    static void eventcb(struct bufferevent *bev, short what, void *ctx);
};
class TorController
{
public:
    TorController(struct event_base* base, const std::string& tor_control_center, const CService& target);
    TorController() : conn{nullptr} {
    }
    ~TorController();
    fs::path GetPrivateKeyFile();
    void Reconnect();
private:
    struct event_base* base;
    const std::string m_tor_control_center;
    TorControlConnection conn;
    std::string private_key;
    std::string service_id;
    bool reconnect;
    struct event *reconnect_ev = nullptr;
    float reconnect_timeout;
    CService service;
    const CService m_target;
    std::vector<uint8_t> cookie;
    std::vector<uint8_t> clientNonce;
public:
    void get_socks_cb(TorControlConnection& conn, const TorControlReply& reply);
    void add_onion_cb(TorControlConnection& conn, const TorControlReply& reply);
    void auth_cb(TorControlConnection& conn, const TorControlReply& reply);
    void authchallenge_cb(TorControlConnection& conn, const TorControlReply& reply);
    void protocolinfo_cb(TorControlConnection& conn, const TorControlReply& reply);
    void connected_cb(TorControlConnection& conn);
    void disconnected_cb(TorControlConnection& conn);
    static void reconnect_cb(evutil_socket_t fd, short what, void *arg);
};
#endif
