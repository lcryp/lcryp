#ifndef LCRYP_UTIL_TOKENPIPE_H
#define LCRYP_UTIL_TOKENPIPE_H
#ifndef WIN32
#include <cstdint>
#include <optional>
class TokenPipeEnd
{
private:
    int m_fd = -1;
public:
    TokenPipeEnd(int fd = -1);
    ~TokenPipeEnd();
    enum Status {
        TS_ERR = -1,
        TS_EOS = -2,
    };
    int TokenWrite(uint8_t token);
    int TokenRead();
    void Close();
    bool IsOpen() { return m_fd != -1; }
    TokenPipeEnd(TokenPipeEnd&& other)
    {
        m_fd = other.m_fd;
        other.m_fd = -1;
    }
    TokenPipeEnd& operator=(TokenPipeEnd&& other)
    {
        Close();
        m_fd = other.m_fd;
        other.m_fd = -1;
        return *this;
    }
    TokenPipeEnd(const TokenPipeEnd&) = delete;
    TokenPipeEnd& operator=(const TokenPipeEnd&) = delete;
};
class TokenPipe
{
private:
    int m_fds[2] = {-1, -1};
    TokenPipe(int fds[2]) : m_fds{fds[0], fds[1]} {}
public:
    ~TokenPipe();
    static std::optional<TokenPipe> Make();
    TokenPipeEnd TakeReadEnd();
    TokenPipeEnd TakeWriteEnd();
    void Close();
    TokenPipe(TokenPipe&& other)
    {
        for (int i = 0; i < 2; ++i) {
            m_fds[i] = other.m_fds[i];
            other.m_fds[i] = -1;
        }
    }
    TokenPipe& operator=(TokenPipe&& other)
    {
        Close();
        for (int i = 0; i < 2; ++i) {
            m_fds[i] = other.m_fds[i];
            other.m_fds[i] = -1;
        }
        return *this;
    }
    TokenPipe(const TokenPipe&) = delete;
    TokenPipe& operator=(const TokenPipe&) = delete;
};
#endif
#endif
