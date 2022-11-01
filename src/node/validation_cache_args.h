#ifndef LCRYP_NODE_VALIDATION_CACHE_ARGS_H
#define LCRYP_NODE_VALIDATION_CACHE_ARGS_H
class ArgsManager;
namespace kernel {
struct ValidationCacheSizes;
};
namespace node {
void ApplyArgsManOptions(const ArgsManager& argsman, kernel::ValidationCacheSizes& cache_sizes);
}
#endif
