#ifndef LCRYP_INDEX_TXINDEX_H
#define LCRYP_INDEX_TXINDEX_H
#include <index/base.h>
class TxIndex final : public BaseIndex
{
protected:
    class DB;
private:
    const std::unique_ptr<DB> m_db;
    bool AllowPrune() const override { return false; }
protected:
    bool CustomAppend(const interfaces::BlockInfo& block) override;
    BaseIndex::DB& GetDB() const override;
public:
    explicit TxIndex(std::unique_ptr<interfaces::Chain> chain, size_t n_cache_size, bool f_memory = false, bool f_wipe = false);
    virtual ~TxIndex() override;
    bool FindTx(const uint256& tx_hash, uint256& block_hash, CTransactionRef& tx) const;
};
extern std::unique_ptr<TxIndex> g_txindex;
#endif
