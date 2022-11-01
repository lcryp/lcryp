#ifndef LCRYP_KERNEL_CHAINSTATEMANAGER_OPTS_H
#define LCRYP_KERNEL_CHAINSTATEMANAGER_OPTS_H
#include <util/time.h>
#include <cstdint>
#include <functional>
class CChainParams;
namespace kernel {
struct ChainstateManagerOpts {
    const CChainParams& chainparams;
    const std::function<NodeClock::time_point()> adjusted_time_callback{nullptr};
};
}
#endif
