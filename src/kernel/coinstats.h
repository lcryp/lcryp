#ifndef LCRYP_KERNEL_COINSTATS_H
#define LCRYP_KERNEL_COINSTATS_H
#include <consensus/amount.h>
#include <streams.h>
#include <uint256.h>
#include <cstdint>
#include <functional>
#include <optional>
class CCoinsView;
class Coin;
class COutPoint;
class CScript;
namespace node {
class BlockManager;
}
namespace kernel {
enum class CoinStatsHashType {
    HASH_SERIALIZED,
    MUHASH,
    NONE,
};
struct CCoinsStats {
    int nHeight{0};
    uint256 hashBlock{};
    uint64_t nTransactions{0};
    uint64_t nTransactionOutputs{0};
    uint64_t nBogoSize{0};
    uint256 hashSerialized{};
    uint64_t nDiskSize{0};
    std::optional<CAmount> total_amount{0};
    uint64_t coins_count{0};
    bool index_used{false};
    CAmount total_subsidy{0};
    CAmount total_unspendable_amount{0};
    CAmount total_prevout_spent_amount{0};
    CAmount total_new_outputs_ex_coinbase_amount{0};
    CAmount total_coinbase_amount{0};
    CAmount total_unspendables_genesis_block{0};
    CAmount total_unspendables_bip30{0};
    CAmount total_unspendables_scripts{0};
    CAmount total_unspendables_unclaimed_rewards{0};
    CCoinsStats() = default;
    CCoinsStats(int block_height, const uint256& block_hash);
};
uint64_t GetBogoSize(const CScript& script_pub_key);
CDataStream TxOutSer(const COutPoint& outpoint, const Coin& coin);
std::optional<CCoinsStats> ComputeUTXOStats(CoinStatsHashType hash_type, CCoinsView* view, node::BlockManager& blockman, const std::function<void()>& interruption_point = {});
}
#endif
