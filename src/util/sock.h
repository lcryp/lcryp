#ifndef LCRYP_UTIL_SOCK_H
#define LCRYP_UTIL_SOCK_H
#include <compat/compat.h>
#include <threadinterrupt.h>
#include <util/time.h>
#include <chrono>
#include <memory>
#include <string>
#include <unordered_map>
static constexpr auto MAX_WAIT_FOR_IO = 1s;
class Sock
{
public:
    Sock();
    explicit Sock(SOCKET s);
    Sock(const Sock&) = delete;
    Sock(Sock&& other);
    virtual ~Sock();
    Sock& operator=(const Sock&) = delete;
    virtual Sock& operator=(Sock&& other);
    [[nodiscard]] virtual SOCKET Get() const;
    [[nodiscard]] virtual ssize_t Send(const void* data, size_t len, int flags) const;
    [[nodiscard]] virtual ssize_t Recv(void* buf, size_t len, int flags) const;
    [[nodiscard]] virtual int Connect(const sockaddr* addr, socklen_t addr_len) const;
    [[nodiscard]] virtual int Bind(const sockaddr* addr, socklen_t addr_len) const;
    [[nodiscard]] virtual int Listen(int backlog) const;
    [[nodiscard]] virtual std::unique_ptr<Sock> Accept(sockaddr* addr, socklen_t* addr_len) const;
    [[nodiscard]] virtual int GetSockOpt(int level,
                                         int opt_name,
                                         void* opt_val,
                                         socklen_t* opt_len) const;
    [[nodiscard]] virtual int SetSockOpt(int level,
                                         int opt_name,
                                         const void* opt_val,
                                         socklen_t opt_len) const;
    [[nodiscard]] virtual int GetSockName(sockaddr* name, socklen_t* name_len) const;
    using Event = uint8_t;
    static constexpr Event RECV = 0b001;
    static constexpr Event SEND = 0b010;
    static constexpr Event ERR = 0b100;
    [[nodiscard]] virtual bool Wait(std::chrono::milliseconds timeout,
                                    Event requested,
                                    Event* occurred = nullptr) const;
    struct Events {
        explicit Events(Event req) : requested{req}, occurred{0} {}
        Event requested;
        Event occurred;
    };
    struct HashSharedPtrSock {
        size_t operator()(const std::shared_ptr<const Sock>& s) const
        {
            return s ? s->m_socket : std::numeric_limits<SOCKET>::max();
        }
    };
    struct EqualSharedPtrSock {
        bool operator()(const std::shared_ptr<const Sock>& lhs,
                        const std::shared_ptr<const Sock>& rhs) const
        {
            if (lhs && rhs) {
                return lhs->m_socket == rhs->m_socket;
            }
            if (!lhs && !rhs) {
                return true;
            }
            return false;
        }
    };
    using EventsPerSock = std::unordered_map<std::shared_ptr<const Sock>, Events, HashSharedPtrSock, EqualSharedPtrSock>;
    [[nodiscard]] virtual bool WaitMany(std::chrono::milliseconds timeout,
                                        EventsPerSock& events_per_sock) const;
    virtual void SendComplete(const std::string& data,
                              std::chrono::milliseconds timeout,
                              CThreadInterrupt& interrupt) const;
    [[nodiscard]] virtual std::string RecvUntilTerminator(uint8_t terminator,
                                                          std::chrono::milliseconds timeout,
                                                          CThreadInterrupt& interrupt,
                                                          size_t max_data) const;
    [[nodiscard]] virtual bool IsConnected(std::string& errmsg) const;
protected:
    SOCKET m_socket;
private:
    void Close();
};
std::string NetworkErrorString(int err);
#endif
