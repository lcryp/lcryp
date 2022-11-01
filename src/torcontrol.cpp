#include <torcontrol.h>
#include <chainparams.h>
#include <chainparamsbase.h>
#include <compat/compat.h>
#include <crypto/hmac_sha256.h>
#include <net.h>
#include <netaddress.h>
#include <netbase.h>
#include <util/readwritefile.h>
#include <util/strencodings.h>
#include <util/syscall_sandbox.h>
#include <util/system.h>
#include <util/thread.h>
#include <util/time.h>
#include <deque>
#include <functional>
#include <set>
#include <vector>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/event.h>
#include <event2/thread.h>
#include <event2/util.h>
const std::string DEFAULT_TOR_CONTROL = "127.0.0.1:9051";
static const int TOR_COOKIE_SIZE = 32;
static const int TOR_NONCE_SIZE = 32;
static const std::string TOR_SAFE_SERVERKEY = "Tor safe cookie authentication server-to-controller hash";
static const std::string TOR_SAFE_CLIENTKEY = "Tor safe cookie authentication controller-to-server hash";
static const float RECONNECT_TIMEOUT_START = 1.0;
static const float RECONNECT_TIMEOUT_EXP = 1.5;
static const int MAX_LINE_LENGTH = 100000;
static const uint16_t DEFAULT_TOR_SOCKS_PORT = 9050;
TorControlConnection::TorControlConnection(struct event_base *_base):
    base(_base), b_conn(nullptr)
{
}
TorControlConnection::~TorControlConnection()
{
    if (b_conn)
        bufferevent_free(b_conn);
}
void TorControlConnection::readcb(struct bufferevent *bev, void *ctx)
{
    TorControlConnection *self = static_cast<TorControlConnection*>(ctx);
    struct evbuffer *input = bufferevent_get_input(bev);
    size_t n_read_out = 0;
    char *line;
    assert(input);
    while((line = evbuffer_readln(input, &n_read_out, EVBUFFER_EOL_CRLF)) != nullptr)
    {
        std::string s(line, n_read_out);
        free(line);
        if (s.size() < 4)
            continue;
        self->message.code = LocaleIndependentAtoi<int>(s.substr(0,3));
        self->message.lines.push_back(s.substr(4));
        char ch = s[3];
        if (ch == ' ') {
            if (self->message.code >= 600) {
                self->async_handler(*self, self->message);
            } else {
                if (!self->reply_handlers.empty()) {
                    self->reply_handlers.front()(*self, self->message);
                    self->reply_handlers.pop_front();
                } else {
                    LogPrint(BCLog::TOR, "Received unexpected sync reply %i\n", self->message.code);
                }
            }
            self->message.Clear();
        }
    }
    if (evbuffer_get_length(input) > MAX_LINE_LENGTH) {
        LogPrintf("tor: Disconnecting because MAX_LINE_LENGTH exceeded\n");
        self->Disconnect();
    }
}
void TorControlConnection::eventcb(struct bufferevent *bev, short what, void *ctx)
{
    TorControlConnection *self = static_cast<TorControlConnection*>(ctx);
    if (what & BEV_EVENT_CONNECTED) {
        LogPrint(BCLog::TOR, "Successfully connected!\n");
        self->connected(*self);
    } else if (what & (BEV_EVENT_EOF|BEV_EVENT_ERROR)) {
        if (what & BEV_EVENT_ERROR) {
            LogPrint(BCLog::TOR, "Error connecting to Tor control socket\n");
        } else {
            LogPrint(BCLog::TOR, "End of stream\n");
        }
        self->Disconnect();
        self->disconnected(*self);
    }
}
bool TorControlConnection::Connect(const std::string& tor_control_center, const ConnectionCB& _connected, const ConnectionCB& _disconnected)
{
    if (b_conn) {
        Disconnect();
    }
    CService control_service;
    if (!Lookup(tor_control_center, control_service, 9051, fNameLookup)) {
        LogPrintf("tor: Failed to look up control center %s\n", tor_control_center);
        return false;
    }
    struct sockaddr_storage control_address;
    socklen_t control_address_len = sizeof(control_address);
    if (!control_service.GetSockAddr(reinterpret_cast<struct sockaddr*>(&control_address), &control_address_len)) {
        LogPrintf("tor: Error parsing socket address %s\n", tor_control_center);
        return false;
    }
    b_conn = bufferevent_socket_new(base, -1, BEV_OPT_CLOSE_ON_FREE);
    if (!b_conn) {
        return false;
    }
    bufferevent_setcb(b_conn, TorControlConnection::readcb, nullptr, TorControlConnection::eventcb, this);
    bufferevent_enable(b_conn, EV_READ|EV_WRITE);
    this->connected = _connected;
    this->disconnected = _disconnected;
    if (bufferevent_socket_connect(b_conn, reinterpret_cast<struct sockaddr*>(&control_address), control_address_len) < 0) {
        LogPrintf("tor: Error connecting to address %s\n", tor_control_center);
        return false;
    }
    return true;
}
void TorControlConnection::Disconnect()
{
    if (b_conn)
        bufferevent_free(b_conn);
    b_conn = nullptr;
}
bool TorControlConnection::Command(const std::string &cmd, const ReplyHandlerCB& reply_handler)
{
    if (!b_conn)
        return false;
    struct evbuffer *buf = bufferevent_get_output(b_conn);
    if (!buf)
        return false;
    evbuffer_add(buf, cmd.data(), cmd.size());
    evbuffer_add(buf, "\r\n", 2);
    reply_handlers.push_back(reply_handler);
    return true;
}
std::pair<std::string,std::string> SplitTorReplyLine(const std::string &s)
{
    size_t ptr=0;
    std::string type;
    while (ptr < s.size() && s[ptr] != ' ') {
        type.push_back(s[ptr]);
        ++ptr;
    }
    if (ptr < s.size())
        ++ptr;
    return make_pair(type, s.substr(ptr));
}
std::map<std::string,std::string> ParseTorReplyMapping(const std::string &s)
{
    std::map<std::string,std::string> mapping;
    size_t ptr=0;
    while (ptr < s.size()) {
        std::string key, value;
        while (ptr < s.size() && s[ptr] != '=' && s[ptr] != ' ') {
            key.push_back(s[ptr]);
            ++ptr;
        }
        if (ptr == s.size())
            return std::map<std::string,std::string>();
        if (s[ptr] == ' ')
            break;
        ++ptr;
        if (ptr < s.size() && s[ptr] == '"') {
            ++ptr;
            bool escape_next = false;
            while (ptr < s.size() && (escape_next || s[ptr] != '"')) {
                escape_next = (s[ptr] == '\\' && !escape_next);
                value.push_back(s[ptr]);
                ++ptr;
            }
            if (ptr == s.size())
                return std::map<std::string,std::string>();
            ++ptr;
            std::string escaped_value;
            for (size_t i = 0; i < value.size(); ++i) {
                if (value[i] == '\\') {
                    ++i;
                    if (value[i] == 'n') {
                        escaped_value.push_back('\n');
                    } else if (value[i] == 't') {
                        escaped_value.push_back('\t');
                    } else if (value[i] == 'r') {
                        escaped_value.push_back('\r');
                    } else if ('0' <= value[i] && value[i] <= '7') {
                        size_t j;
                        for (j = 1; j < 3 && (i+j) < value.size() && '0' <= value[i+j] && value[i+j] <= '7'; ++j) {}
                        if (j == 3 && value[i] > '3') {
                            j--;
                        }
                        const auto end{i + j};
                        uint8_t val{0};
                        while (i < end) {
                            val *= 8;
                            val += value[i++] - '0';
                        }
                        escaped_value.push_back(char(val));
                        --i;
                    } else {
                        escaped_value.push_back(value[i]);
                    }
                } else {
                    escaped_value.push_back(value[i]);
                }
            }
            value = escaped_value;
        } else {
            while (ptr < s.size() && s[ptr] != ' ') {
                value.push_back(s[ptr]);
                ++ptr;
            }
        }
        if (ptr < s.size() && s[ptr] == ' ')
            ++ptr;
        mapping[key] = value;
    }
    return mapping;
}
TorController::TorController(struct event_base* _base, const std::string& tor_control_center, const CService& target):
    base(_base),
    m_tor_control_center(tor_control_center), conn(base), reconnect(true), reconnect_timeout(RECONNECT_TIMEOUT_START),
    m_target(target)
{
    reconnect_ev = event_new(base, -1, 0, reconnect_cb, this);
    if (!reconnect_ev)
        LogPrintf("tor: Failed to create event for reconnection: out of memory?\n");
    if (!conn.Connect(m_tor_control_center, std::bind(&TorController::connected_cb, this, std::placeholders::_1),
         std::bind(&TorController::disconnected_cb, this, std::placeholders::_1) )) {
        LogPrintf("tor: Initiating connection to Tor control port %s failed\n", m_tor_control_center);
    }
    std::pair<bool,std::string> pkf = ReadBinaryFile(GetPrivateKeyFile());
    if (pkf.first) {
        LogPrint(BCLog::TOR, "Reading cached private key from %s\n", fs::PathToString(GetPrivateKeyFile()));
        private_key = pkf.second;
    }
}
TorController::~TorController()
{
    if (reconnect_ev) {
        event_free(reconnect_ev);
        reconnect_ev = nullptr;
    }
    if (service.IsValid()) {
        RemoveLocal(service);
    }
}
void TorController::get_socks_cb(TorControlConnection& _conn, const TorControlReply& reply)
{
    std::string socks_location;
    if (reply.code == 250) {
        for (const auto& line : reply.lines) {
            if (0 == line.compare(0, 20, "net/listeners/socks=")) {
                const std::string port_list_str = line.substr(20);
                std::vector<std::string> port_list = SplitString(port_list_str, ' ');
                for (auto& portstr : port_list) {
                    if (portstr.empty()) continue;
                    if ((portstr[0] == '"' || portstr[0] == '\'') && portstr.size() >= 2 && (*portstr.rbegin() == portstr[0])) {
                        portstr = portstr.substr(1, portstr.size() - 2);
                        if (portstr.empty()) continue;
                    }
                    socks_location = portstr;
                    if (0 == portstr.compare(0, 10, "127.0.0.1:")) {
                        break;
                    }
                }
            }
        }
        if (!socks_location.empty()) {
            LogPrint(BCLog::TOR, "Get SOCKS port command yielded %s\n", socks_location);
        } else {
            LogPrintf("tor: Get SOCKS port command returned nothing\n");
        }
    } else if (reply.code == 510) {
        LogPrintf("tor: Get SOCKS port command failed with unrecognized command (You probably should upgrade Tor)\n");
    } else {
        LogPrintf("tor: Get SOCKS port command failed; error code %d\n", reply.code);
    }
    CService resolved;
    Assume(!resolved.IsValid());
    if (!socks_location.empty()) {
        resolved = LookupNumeric(socks_location, DEFAULT_TOR_SOCKS_PORT);
    }
    if (!resolved.IsValid()) {
        resolved = LookupNumeric("127.0.0.1", DEFAULT_TOR_SOCKS_PORT);
    }
    Assume(resolved.IsValid());
    LogPrint(BCLog::TOR, "Configuring onion proxy for %s\n", resolved.ToStringIPPort());
    Proxy addrOnion = Proxy(resolved, true);
    SetProxy(NET_ONION, addrOnion);
    const auto onlynets = gArgs.GetArgs("-onlynet");
    const bool onion_allowed_by_onlynet{
        !gArgs.IsArgSet("-onlynet") ||
        std::any_of(onlynets.begin(), onlynets.end(), [](const auto& n) {
            return ParseNetwork(n) == NET_ONION;
        })};
    if (onion_allowed_by_onlynet) {
        SetReachable(NET_ONION, true);
    }
}
void TorController::add_onion_cb(TorControlConnection& _conn, const TorControlReply& reply)
{
    if (reply.code == 250) {
        LogPrint(BCLog::TOR, "ADD_ONION successful\n");
        for (const std::string &s : reply.lines) {
            std::map<std::string,std::string> m = ParseTorReplyMapping(s);
            std::map<std::string,std::string>::iterator i;
            if ((i = m.find("ServiceID")) != m.end())
                service_id = i->second;
            if ((i = m.find("PrivateKey")) != m.end())
                private_key = i->second;
        }
        if (service_id.empty()) {
            LogPrintf("tor: Error parsing ADD_ONION parameters:\n");
            for (const std::string &s : reply.lines) {
                LogPrintf("    %s\n", SanitizeString(s));
            }
            return;
        }
        service = LookupNumeric(std::string(service_id+".onion"), Params().GetDefaultPort());
        LogPrintfCategory(BCLog::TOR, "Got service ID %s, advertising service %s\n", service_id, service.ToString());
        if (WriteBinaryFile(GetPrivateKeyFile(), private_key)) {
            LogPrint(BCLog::TOR, "Cached service private key to %s\n", fs::PathToString(GetPrivateKeyFile()));
        } else {
            LogPrintf("tor: Error writing service private key to %s\n", fs::PathToString(GetPrivateKeyFile()));
        }
        AddLocal(service, LOCAL_MANUAL);
    } else if (reply.code == 510) {
        LogPrintf("tor: Add onion failed with unrecognized command (You probably need to upgrade Tor)\n");
    } else {
        LogPrintf("tor: Add onion failed; error code %d\n", reply.code);
    }
}
void TorController::auth_cb(TorControlConnection& _conn, const TorControlReply& reply)
{
    if (reply.code == 250) {
        LogPrint(BCLog::TOR, "Authentication successful\n");
        if (gArgs.GetArg("-onion", "") == "") {
            _conn.Command("GETINFO net/listeners/socks", std::bind(&TorController::get_socks_cb, this, std::placeholders::_1, std::placeholders::_2));
        }
        if (private_key.empty()) {
            private_key = "NEW:ED25519-V3";
        }
        _conn.Command(strprintf("ADD_ONION %s Port=%i,%s", private_key, Params().GetDefaultPort(), m_target.ToStringIPPort()),
            std::bind(&TorController::add_onion_cb, this, std::placeholders::_1, std::placeholders::_2));
    } else {
        LogPrintf("tor: Authentication failed\n");
    }
}
static std::vector<uint8_t> ComputeResponse(const std::string &key, const std::vector<uint8_t> &cookie,  const std::vector<uint8_t> &clientNonce, const std::vector<uint8_t> &serverNonce)
{
    CHMAC_SHA256 computeHash((const uint8_t*)key.data(), key.size());
    std::vector<uint8_t> computedHash(CHMAC_SHA256::OUTPUT_SIZE, 0);
    computeHash.Write(cookie.data(), cookie.size());
    computeHash.Write(clientNonce.data(), clientNonce.size());
    computeHash.Write(serverNonce.data(), serverNonce.size());
    computeHash.Finalize(computedHash.data());
    return computedHash;
}
void TorController::authchallenge_cb(TorControlConnection& _conn, const TorControlReply& reply)
{
    if (reply.code == 250) {
        LogPrint(BCLog::TOR, "SAFECOOKIE authentication challenge successful\n");
        std::pair<std::string,std::string> l = SplitTorReplyLine(reply.lines[0]);
        if (l.first == "AUTHCHALLENGE") {
            std::map<std::string,std::string> m = ParseTorReplyMapping(l.second);
            if (m.empty()) {
                LogPrintf("tor: Error parsing AUTHCHALLENGE parameters: %s\n", SanitizeString(l.second));
                return;
            }
            std::vector<uint8_t> serverHash = ParseHex(m["SERVERHASH"]);
            std::vector<uint8_t> serverNonce = ParseHex(m["SERVERNONCE"]);
            LogPrint(BCLog::TOR, "AUTHCHALLENGE ServerHash %s ServerNonce %s\n", HexStr(serverHash), HexStr(serverNonce));
            if (serverNonce.size() != 32) {
                LogPrintf("tor: ServerNonce is not 32 bytes, as required by spec\n");
                return;
            }
            std::vector<uint8_t> computedServerHash = ComputeResponse(TOR_SAFE_SERVERKEY, cookie, clientNonce, serverNonce);
            if (computedServerHash != serverHash) {
                LogPrintf("tor: ServerHash %s does not match expected ServerHash %s\n", HexStr(serverHash), HexStr(computedServerHash));
                return;
            }
            std::vector<uint8_t> computedClientHash = ComputeResponse(TOR_SAFE_CLIENTKEY, cookie, clientNonce, serverNonce);
            _conn.Command("AUTHENTICATE " + HexStr(computedClientHash), std::bind(&TorController::auth_cb, this, std::placeholders::_1, std::placeholders::_2));
        } else {
            LogPrintf("tor: Invalid reply to AUTHCHALLENGE\n");
        }
    } else {
        LogPrintf("tor: SAFECOOKIE authentication challenge failed\n");
    }
}
void TorController::protocolinfo_cb(TorControlConnection& _conn, const TorControlReply& reply)
{
    if (reply.code == 250) {
        std::set<std::string> methods;
        std::string cookiefile;
        for (const std::string &s : reply.lines) {
            std::pair<std::string,std::string> l = SplitTorReplyLine(s);
            if (l.first == "AUTH") {
                std::map<std::string,std::string> m = ParseTorReplyMapping(l.second);
                std::map<std::string,std::string>::iterator i;
                if ((i = m.find("METHODS")) != m.end()) {
                    std::vector<std::string> m_vec = SplitString(i->second, ',');
                    methods = std::set<std::string>(m_vec.begin(), m_vec.end());
                }
                if ((i = m.find("COOKIEFILE")) != m.end())
                    cookiefile = i->second;
            } else if (l.first == "VERSION") {
                std::map<std::string,std::string> m = ParseTorReplyMapping(l.second);
                std::map<std::string,std::string>::iterator i;
                if ((i = m.find("Tor")) != m.end()) {
                    LogPrint(BCLog::TOR, "Connected to Tor version %s\n", i->second);
                }
            }
        }
        for (const std::string &s : methods) {
            LogPrint(BCLog::TOR, "Supported authentication method: %s\n", s);
        }
        std::string torpassword = gArgs.GetArg("-torpassword", "");
        if (!torpassword.empty()) {
            if (methods.count("HASHEDPASSWORD")) {
                LogPrint(BCLog::TOR, "Using HASHEDPASSWORD authentication\n");
                ReplaceAll(torpassword, "\"", "\\\"");
                _conn.Command("AUTHENTICATE \"" + torpassword + "\"", std::bind(&TorController::auth_cb, this, std::placeholders::_1, std::placeholders::_2));
            } else {
                LogPrintf("tor: Password provided with -torpassword, but HASHEDPASSWORD authentication is not available\n");
            }
        } else if (methods.count("NULL")) {
            LogPrint(BCLog::TOR, "Using NULL authentication\n");
            _conn.Command("AUTHENTICATE", std::bind(&TorController::auth_cb, this, std::placeholders::_1, std::placeholders::_2));
        } else if (methods.count("SAFECOOKIE")) {
            LogPrint(BCLog::TOR, "Using SAFECOOKIE authentication, reading cookie authentication from %s\n", cookiefile);
            std::pair<bool,std::string> status_cookie = ReadBinaryFile(fs::PathFromString(cookiefile), TOR_COOKIE_SIZE);
            if (status_cookie.first && status_cookie.second.size() == TOR_COOKIE_SIZE) {
                cookie = std::vector<uint8_t>(status_cookie.second.begin(), status_cookie.second.end());
                clientNonce = std::vector<uint8_t>(TOR_NONCE_SIZE, 0);
                GetRandBytes(clientNonce);
                _conn.Command("AUTHCHALLENGE SAFECOOKIE " + HexStr(clientNonce), std::bind(&TorController::authchallenge_cb, this, std::placeholders::_1, std::placeholders::_2));
            } else {
                if (status_cookie.first) {
                    LogPrintf("tor: Authentication cookie %s is not exactly %i bytes, as is required by the spec\n", cookiefile, TOR_COOKIE_SIZE);
                } else {
                    LogPrintf("tor: Authentication cookie %s could not be opened (check permissions)\n", cookiefile);
                }
            }
        } else if (methods.count("HASHEDPASSWORD")) {
            LogPrintf("tor: The only supported authentication mechanism left is password, but no password provided with -torpassword\n");
        } else {
            LogPrintf("tor: No supported authentication method\n");
        }
    } else {
        LogPrintf("tor: Requesting protocol info failed\n");
    }
}
void TorController::connected_cb(TorControlConnection& _conn)
{
    reconnect_timeout = RECONNECT_TIMEOUT_START;
    if (!_conn.Command("PROTOCOLINFO 1", std::bind(&TorController::protocolinfo_cb, this, std::placeholders::_1, std::placeholders::_2)))
        LogPrintf("tor: Error sending initial protocolinfo command\n");
}
void TorController::disconnected_cb(TorControlConnection& _conn)
{
    if (service.IsValid())
        RemoveLocal(service);
    service = CService();
    if (!reconnect)
        return;
    LogPrint(BCLog::TOR, "Not connected to Tor control port %s, trying to reconnect\n", m_tor_control_center);
    struct timeval time = MillisToTimeval(int64_t(reconnect_timeout * 1000.0));
    if (reconnect_ev)
        event_add(reconnect_ev, &time);
    reconnect_timeout *= RECONNECT_TIMEOUT_EXP;
}
void TorController::Reconnect()
{
    if (!conn.Connect(m_tor_control_center, std::bind(&TorController::connected_cb, this, std::placeholders::_1),
         std::bind(&TorController::disconnected_cb, this, std::placeholders::_1) )) {
        LogPrintf("tor: Re-initiating connection to Tor control port %s failed\n", m_tor_control_center);
    }
}
fs::path TorController::GetPrivateKeyFile()
{
    return gArgs.GetDataDirNet() / "onion_v3_private_key";
}
void TorController::reconnect_cb(evutil_socket_t fd, short what, void *arg)
{
    TorController *self = static_cast<TorController*>(arg);
    self->Reconnect();
}
static struct event_base *gBase;
static std::thread torControlThread;
static void TorControlThread(CService onion_service_target)
{
    SetSyscallSandboxPolicy(SyscallSandboxPolicy::TOR_CONTROL);
    TorController ctrl(gBase, gArgs.GetArg("-torcontrol", DEFAULT_TOR_CONTROL), onion_service_target);
    event_base_dispatch(gBase);
}
void StartTorControl(CService onion_service_target)
{
    assert(!gBase);
#ifdef WIN32
    evthread_use_windows_threads();
#else
    evthread_use_pthreads();
#endif
    gBase = event_base_new();
    if (!gBase) {
        LogPrintf("tor: Unable to create event_base\n");
        return;
    }
    torControlThread = std::thread(&util::TraceThread, "torcontrol", [onion_service_target] {
        TorControlThread(onion_service_target);
    });
}
void InterruptTorControl()
{
    if (gBase) {
        LogPrintf("tor: Thread interrupt\n");
        event_base_once(gBase, -1, EV_TIMEOUT, [](evutil_socket_t, short, void*) {
            event_base_loopbreak(gBase);
        }, nullptr, nullptr);
    }
}
void StopTorControl()
{
    if (gBase) {
        torControlThread.join();
        event_base_free(gBase);
        gBase = nullptr;
    }
}
CService DefaultOnionServiceTarget()
{
    struct in_addr onion_service_target;
    onion_service_target.s_addr = htonl(INADDR_LOOPBACK);
    return {onion_service_target, BaseParams().OnionServiceTargetPort()};
}
