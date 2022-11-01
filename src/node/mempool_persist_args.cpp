#include <node/mempool_persist_args.h>
#include <fs.h>
#include <util/system.h>
#include <validation.h>
namespace node {
bool ShouldPersistMempool(const ArgsManager& argsman)
{
    return argsman.GetBoolArg("-persistmempool", DEFAULT_PERSIST_MEMPOOL);
}
fs::path MempoolPath(const ArgsManager& argsman)
{
    return argsman.GetDataDirNet() / "mempool.dat";
}
}
