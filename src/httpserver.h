#ifndef LCRYP_HTTPSERVER_H
#define LCRYP_HTTPSERVER_H
#include <functional>
#include <optional>
#include <string>
static const int DEFAULT_HTTP_THREADS=4;
static const int DEFAULT_HTTP_WORKQUEUE=16;
static const int DEFAULT_HTTP_SERVER_TIMEOUT=30;
struct evhttp_request;
struct event_base;
class CService;
class HTTPRequest;
bool InitHTTPServer();
void StartHTTPServer();
void InterruptHTTPServer();
void StopHTTPServer();
void UpdateHTTPServerLogging(bool enable);
typedef std::function<bool(HTTPRequest* req, const std::string &)> HTTPRequestHandler;
void RegisterHTTPHandler(const std::string &prefix, bool exactMatch, const HTTPRequestHandler &handler);
void UnregisterHTTPHandler(const std::string &prefix, bool exactMatch);
struct event_base* EventBase();
class HTTPRequest
{
private:
    struct evhttp_request* req;
    bool replySent;
public:
    explicit HTTPRequest(struct evhttp_request* req, bool replySent = false);
    ~HTTPRequest();
    enum RequestMethod {
        UNKNOWN,
        GET,
        POST,
        HEAD,
        PUT
    };
    std::string GetURI() const;
    CService GetPeer() const;
    RequestMethod GetRequestMethod() const;
    std::optional<std::string> GetQueryParameter(const std::string& key) const;
    std::pair<bool, std::string> GetHeader(const std::string& hdr) const;
    std::string ReadBody();
    void WriteHeader(const std::string& hdr, const std::string& value);
    void WriteReply(int nStatus, const std::string& strReply = "");
};
std::optional<std::string> GetQueryParameterFromUri(const char* uri, const std::string& key);
class HTTPClosure
{
public:
    virtual void operator()() = 0;
    virtual ~HTTPClosure() {}
};
class HTTPEvent
{
public:
    HTTPEvent(struct event_base* base, bool deleteWhenTriggered, const std::function<void()>& handler);
    ~HTTPEvent();
    void trigger(struct timeval* tv);
    bool deleteWhenTriggered;
    std::function<void()> handler;
private:
    struct event* ev;
};
#endif
