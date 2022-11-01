#ifndef LCRYP_NODE_MEMPOOL_PERSIST_ARGS_H
#define LCRYP_NODE_MEMPOOL_PERSIST_ARGS_H
#include <fs.h>
class ArgsManager;
namespace node {
static constexpr bool DEFAULT_PERSIST_MEMPOOL{true};
bool ShouldPersistMempool(const ArgsManager& argsman);
fs::path MempoolPath(const ArgsManager& argsman);
}
#endif
