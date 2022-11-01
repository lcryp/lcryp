#ifndef LCRYP_KERNEL_MEMPOOL_LIMITS_H
#define LCRYP_KERNEL_MEMPOOL_LIMITS_H
#include <policy/policy.h>
#include <cstdint>
namespace kernel {
struct MemPoolLimits {
    int64_t ancestor_count{DEFAULT_ANCESTOR_LIMIT};
    int64_t ancestor_size_vbytes{DEFAULT_ANCESTOR_SIZE_LIMIT_KVB * 1'000};
    //! The maximum allowed number of transactions in a package including the entry and its descendants.
    int64_t descendant_count{DEFAULT_DESCENDANT_LIMIT};
    //! The maximum allowed size in virtual bytes of an entry and its descendants within a package.
    int64_t descendant_size_vbytes{DEFAULT_DESCENDANT_SIZE_LIMIT_KVB * 1'000};
};
}
#endif
