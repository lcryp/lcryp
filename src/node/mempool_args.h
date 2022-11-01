#ifndef LCRYP_NODE_MEMPOOL_ARGS_H
#define LCRYP_NODE_MEMPOOL_ARGS_H
#include <optional>
class ArgsManager;
class CChainParams;
struct bilingual_str;
namespace kernel {
struct MemPoolOptions;
};
[[nodiscard]] std::optional<bilingual_str> ApplyArgsManOptions(const ArgsManager& argsman, const CChainParams& chainparams, kernel::MemPoolOptions& mempool_opts);
#endif
