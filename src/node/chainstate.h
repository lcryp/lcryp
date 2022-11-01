#ifndef LCRYP_NODE_CHAINSTATE_H
#define LCRYP_NODE_CHAINSTATE_H
#include <util/translation.h>
#include <validation.h>
#include <cstdint>
#include <functional>
#include <tuple>
class CTxMemPool;
namespace node {
struct CacheSizes;
struct ChainstateLoadOptions {
    CTxMemPool* mempool{nullptr};
    bool block_tree_db_in_memory{false};
    bool coins_db_in_memory{false};
    bool reindex{false};
    bool reindex_chainstate{false};
    bool prune{false};
    int64_t check_blocks{DEFAULT_CHECKBLOCKS};
    int64_t check_level{DEFAULT_CHECKLEVEL};
    std::function<bool()> check_interrupt;
    std::function<void()> coins_error_cb;
};
enum class ChainstateLoadStatus { SUCCESS, FAILURE, FAILURE_INCOMPATIBLE_DB, INTERRUPTED };
using ChainstateLoadResult = std::tuple<ChainstateLoadStatus, bilingual_str>;
ChainstateLoadResult LoadChainstate(ChainstateManager& chainman, const CacheSizes& cache_sizes,
                                    const ChainstateLoadOptions& options);
ChainstateLoadResult VerifyLoadedChainstate(ChainstateManager& chainman, const ChainstateLoadOptions& options);
}
#endif
