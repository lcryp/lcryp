#ifndef LCRYP_INTERFACES_IPC_H
#define LCRYP_INTERFACES_IPC_H
#include <functional>
#include <memory>
#include <typeindex>
namespace ipc {
struct Context;
}
namespace interfaces {
class Init;
class Ipc
{
public:
    virtual ~Ipc() = default;
    virtual std::unique_ptr<Init> spawnProcess(const char* exe_name) = 0;
    virtual bool startSpawnedProcess(int argc, char* argv[], int& exit_status) = 0;
    template<typename Interface>
    void addCleanup(Interface& iface, std::function<void()> cleanup)
    {
        addCleanup(typeid(Interface), &iface, std::move(cleanup));
    }
    virtual ipc::Context& context() = 0;
protected:
    virtual void addCleanup(std::type_index type, void* iface, std::function<void()> cleanup) = 0;
};
std::unique_ptr<Ipc> MakeIpc(const char* exe_name, const char* process_argv0, Init& init);
}
#endif
