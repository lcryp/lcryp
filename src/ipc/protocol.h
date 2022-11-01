#ifndef LCRYP_IPC_PROTOCOL_H
#define LCRYP_IPC_PROTOCOL_H
#include <interfaces/init.h>
#include <functional>
#include <memory>
#include <typeindex>
namespace ipc {
struct Context;
class Protocol
{
public:
    virtual ~Protocol() = default;
    virtual std::unique_ptr<interfaces::Init> connect(int fd, const char* exe_name) = 0;
    virtual void serve(int fd, const char* exe_name, interfaces::Init& init) = 0;
    virtual void addCleanup(std::type_index type, void* iface, std::function<void()> cleanup) = 0;
    virtual Context& context() = 0;
};
}
#endif
