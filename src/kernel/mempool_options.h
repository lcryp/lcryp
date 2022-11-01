#ifndef LCRYP_KERNEL_MEMPOOL_OPTIONS_H
#define LCRYP_KERNEL_MEMPOOL_OPTIONS_H
#include <kernel/mempool_limits.h>
#include <policy/feerate.h>
#include <policy/policy.h>
#include <script/standard.h>
#include <chrono>
#include <cstdint>
#include <optional>
class CBlockPolicyEstimator;
static constexpr unsigned int DEFAULT_MAX_MEMPOOL_SIZE_MB{300};
static constexpr unsigned int DEFAULT_MEMPOOL_EXPIRY_HOURS{336};
static constexpr bool DEFAULT_MEMPOOL_FULL_RBF{false};
namespace kernel {
struct MemPoolOptions {
    CBlockPolicyEstimator* estimator{nullptr};
    int check_ratio{0};
    int64_t max_size_bytes{DEFAULT_MAX_MEMPOOL_SIZE_MB * 1'000'000};
    std::chrono::seconds expiry{std::chrono::hours{DEFAULT_MEMPOOL_EXPIRY_HOURS}};
    CFeeRate incremental_relay_feerate{DEFAULT_INCREMENTAL_RELAY_FEE};
    CFeeRate min_relay_feerate{DEFAULT_MIN_RELAY_TX_FEE};
    CFeeRate dust_relay_feerate{DUST_RELAY_TX_FEE};
    std::optional<unsigned> max_datacarrier_bytes{DEFAULT_ACCEPT_DATACARRIER ? std::optional{MAX_OP_RETURN_RELAY} : std::nullopt};
    bool permit_bare_multisig{DEFAULT_PERMIT_BAREMULTISIG};
    bool require_standard{true};
    bool full_rbf{DEFAULT_MEMPOOL_FULL_RBF};
    MemPoolLimits limits{};
};
}
#endif
