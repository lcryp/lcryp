#ifndef LCRYP_INDEX_COINSTATSINDEX_H
#define LCRYP_INDEX_COINSTATSINDEX_H
#include <crypto/muhash.h>
#include <index/base.h>
class CBlockIndex;
class CDBBatch;
namespace kernel {
struct CCoinsStats;
}
class CoinStatsIndex final : public BaseIndex
{
private:
    std::unique_ptr<BaseIndex::DB> m_db;
    MuHash3072 m_muhash;
    uint64_t m_transaction_output_count{0};
    uint64_t m_bogo_size{0};
    CAmount m_total_amount{0};
    CAmount m_total_subsidy{0};
    CAmount m_total_unspendable_amount{0};
    CAmount m_total_prevout_spent_amount{0};
    CAmount m_total_new_outputs_ex_coinbase_amount{0};
    CAmount m_total_coinbase_amount{0};
    CAmount m_total_unspendables_genesis_block{0};
    CAmount m_total_unspendables_bip30{0};
    CAmount m_total_unspendables_scripts{0};
    CAmount m_total_unspendables_unclaimed_rewards{0};
    bool ReverseBlock(const CBlock& block, const CBlockIndex* pindex);
    bool AllowPrune() const override { return true; }
protected:
    bool CustomInit(const std::optional<interfaces::BlockKey>& block) override;
    bool CustomCommit(CDBBatch& batch) override;
    bool CustomAppend(const interfaces::BlockInfo& block) override;
    bool CustomRewind(const interfaces::BlockKey& current_tip, const interfaces::BlockKey& new_tip) override;
    BaseIndex::DB& GetDB() const override { return *m_db; }
public:
    explicit CoinStatsIndex(std::unique_ptr<interfaces::Chain> chain, size_t n_cache_size, bool f_memory = false, bool f_wipe = false);
    std::optional<kernel::CCoinsStats> LookUpStats(const CBlockIndex& block_index) const;
};
extern std::unique_ptr<CoinStatsIndex> g_coin_stats_index;
#endif
