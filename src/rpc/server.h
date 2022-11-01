#ifndef LCRYP_RPC_SERVER_H
#define LCRYP_RPC_SERVER_H
#include <rpc/request.h>
#include <rpc/util.h>
#include <functional>
#include <map>
#include <stdint.h>
#include <string>
#include <univalue.h>
static const unsigned int DEFAULT_RPC_SERIALIZE_VERSION = 1;
class CRPCCommand;
namespace RPCServer
{
    void OnStarted(std::function<void ()> slot);
    void OnStopped(std::function<void ()> slot);
}
bool IsRPCRunning();
void RpcInterruptionPoint();
void SetRPCWarmupStatus(const std::string& newStatus);
void SetRPCWarmupFinished();
bool RPCIsInWarmup(std::string *outStatus);
class RPCTimerBase
{
public:
    virtual ~RPCTimerBase() {}
};
class RPCTimerInterface
{
public:
    virtual ~RPCTimerInterface() {}
    virtual const char *Name() = 0;
    virtual RPCTimerBase* NewTimer(std::function<void()>& func, int64_t millis) = 0;
};
void RPCSetTimerInterface(RPCTimerInterface *iface);
void RPCSetTimerInterfaceIfUnset(RPCTimerInterface *iface);
void RPCUnsetTimerInterface(RPCTimerInterface *iface);
void RPCRunLater(const std::string& name, std::function<void()> func, int64_t nSeconds);
typedef RPCHelpMan (*RpcMethodFnType)();
class CRPCCommand
{
public:
    using Actor = std::function<bool(const JSONRPCRequest& request, UniValue& result, bool last_handler)>;
    CRPCCommand(std::string category, std::string name, Actor actor, std::vector<std::string> args, intptr_t unique_id)
        : category(std::move(category)), name(std::move(name)), actor(std::move(actor)), argNames(std::move(args)),
          unique_id(unique_id)
    {
    }
    CRPCCommand(std::string category, RpcMethodFnType fn)
        : CRPCCommand(
              category,
              fn().m_name,
              [fn](const JSONRPCRequest& request, UniValue& result, bool) { result = fn().HandleRequest(request); return true; },
              fn().GetArgNames(),
              intptr_t(fn))
    {
    }
    std::string category;
    std::string name;
    Actor actor;
    std::vector<std::string> argNames;
    intptr_t unique_id;
};
class CRPCTable
{
private:
    std::map<std::string, std::vector<const CRPCCommand*>> mapCommands;
public:
    CRPCTable();
    std::string help(const std::string& name, const JSONRPCRequest& helpreq) const;
    UniValue execute(const JSONRPCRequest &request) const;
    std::vector<std::string> listCommands() const;
    UniValue dumpArgMap(const JSONRPCRequest& request) const;
    void appendCommand(const std::string& name, const CRPCCommand* pcmd);
    bool removeCommand(const std::string& name, const CRPCCommand* pcmd);
};
bool IsDeprecatedRPCEnabled(const std::string& method);
extern CRPCTable tableRPC;
void StartRPC();
void InterruptRPC();
void StopRPC();
std::string JSONRPCExecBatch(const JSONRPCRequest& jreq, const UniValue& vReq);
int RPCSerializationFlags();
#endif
