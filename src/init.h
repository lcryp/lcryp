#ifndef LCRYP_INIT_H
#define LCRYP_INIT_H
#include <any>
#include <memory>
#include <string>
static constexpr bool DEFAULT_DAEMON = false;
static constexpr bool DEFAULT_DAEMONWAIT = false;
class ArgsManager;
namespace interfaces {
struct BlockAndHeaderTipInfo;
}
namespace kernel {
struct Context;
}
namespace node {
struct NodeContext;
}
void Interrupt(node::NodeContext& node);
void Shutdown(node::NodeContext& node);
void InitLogging(const ArgsManager& args);
void InitParameterInteraction(ArgsManager& args);
bool AppInitBasicSetup(const ArgsManager& args);
bool AppInitParameterInteraction(const ArgsManager& args, bool use_syscall_sandbox = true);
bool AppInitSanityChecks(const kernel::Context& kernel);
bool AppInitLockDataDirectory();
bool AppInitInterfaces(node::NodeContext& node);
bool AppInitMain(node::NodeContext& node, interfaces::BlockAndHeaderTipInfo* tip_info = nullptr);
void SetupServerArgs(ArgsManager& argsman);
#endif
