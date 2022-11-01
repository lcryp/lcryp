#ifndef LCRYP_NODE_CACHES_H
#define LCRYP_NODE_CACHES_H
#include <cstddef>
#include <cstdint>
class ArgsManager;
namespace node {
struct CacheSizes {
    int64_t block_tree_db;
    int64_t coins_db;
    int64_t coins;
    int64_t tx_index;
    int64_t filter_index;
};
CacheSizes CalculateCacheSizes(const ArgsManager& args, size_t n_indexes = 0);
}
#endif
