#include <node/validation_cache_args.h>
#include <kernel/validation_cache_sizes.h>
#include <util/system.h>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
using kernel::ValidationCacheSizes;
namespace node {
void ApplyArgsManOptions(const ArgsManager& argsman, ValidationCacheSizes& cache_sizes)
{
    if (auto max_size = argsman.GetIntArg("-maxsigcachesize")) {
        size_t clamped_size_each = std::max<int64_t>(*max_size, 0) * (1 << 20) / 2;
        cache_sizes = {
            .signature_cache_bytes = clamped_size_each,
            .script_execution_cache_bytes = clamped_size_each,
        };
    }
}
}
