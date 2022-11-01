#ifndef LCRYP_I2P_H
#define LCRYP_I2P_H
#include <compat/compat.h>
#include <fs.h>
#include <netaddress.h>
#include <sync.h>
#include <threadinterrupt.h>
#include <util/sock.h>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>
namespace i2p {
using Binary = std::vector<uint8_t>;
struct Connection {
    std::unique_ptr<Sock> sock;
    CService me;
    CService peer;
};
namespace sam {
static constexpr size_t MAX_MSG_SIZE{65536};
class Session
{
public:
    Session(const fs::path& private_key_file,
            const CService& control_host,
            CThreadInterrupt* interrupt);
    Session(const CService& control_host, CThreadInterrupt* interrupt);
    ~Session();
    bool Listen(Connection& conn) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);
    bool Accept(Connection& conn);
    bool Connect(const CService& to, Connection& conn, bool& proxy_error) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);
private:
    struct Reply {
        std::string full;
        std::string request;
        std::unordered_map<std::string, std::optional<std::string>> keys;
        std::string Get(const std::string& key) const;
    };
    template <typename... Args>
    void Log(const std::string& fmt, const Args&... args) const;
    Reply SendRequestAndGetReply(const Sock& sock,
                                 const std::string& request,
                                 bool check_result_ok = true) const;
    std::unique_ptr<Sock> Hello() const EXCLUSIVE_LOCKS_REQUIRED(m_mutex);
    void CheckControlSock() EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);
    void DestGenerate(const Sock& sock) EXCLUSIVE_LOCKS_REQUIRED(m_mutex);
    void GenerateAndSavePrivateKey(const Sock& sock) EXCLUSIVE_LOCKS_REQUIRED(m_mutex);
    Binary MyDestination() const EXCLUSIVE_LOCKS_REQUIRED(m_mutex);
    void CreateIfNotCreatedAlready() EXCLUSIVE_LOCKS_REQUIRED(m_mutex);
    std::unique_ptr<Sock> StreamAccept() EXCLUSIVE_LOCKS_REQUIRED(m_mutex);
    void Disconnect() EXCLUSIVE_LOCKS_REQUIRED(m_mutex);
    const fs::path m_private_key_file;
    const CService m_control_host;
    CThreadInterrupt* const m_interrupt;
    mutable Mutex m_mutex;
    Binary m_private_key GUARDED_BY(m_mutex);
    std::unique_ptr<Sock> m_control_sock GUARDED_BY(m_mutex);
    CService m_my_addr GUARDED_BY(m_mutex);
    std::string m_session_id GUARDED_BY(m_mutex);
    const bool m_transient;
};
}
}
#endif
