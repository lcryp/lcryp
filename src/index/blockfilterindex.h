#ifndef LCRYP_INDEX_BLOCKFILTERINDEX_H
#define LCRYP_INDEX_BLOCKFILTERINDEX_H
#include <attributes.h>
#include <blockfilter.h>
#include <chain.h>
#include <flatfile.h>
#include <index/base.h>
#include <util/hasher.h>
static constexpr int CFCHECKPT_INTERVAL = 1000;
class BlockFilterIndex final : public BaseIndex
{
private:
    BlockFilterType m_filter_type;
    std::unique_ptr<BaseIndex::DB> m_db;
    FlatFilePos m_next_filter_pos;
    std::unique_ptr<FlatFileSeq> m_filter_fileseq;
    bool ReadFilterFromDisk(const FlatFilePos& pos, const uint256& hash, BlockFilter& filter) const;
    size_t WriteFilterToDisk(FlatFilePos& pos, const BlockFilter& filter);
    Mutex m_cs_headers_cache;
    std::unordered_map<uint256, uint256, FilterHeaderHasher> m_headers_cache GUARDED_BY(m_cs_headers_cache);
    bool AllowPrune() const override { return true; }
protected:
    bool CustomInit(const std::optional<interfaces::BlockKey>& block) override;
    bool CustomCommit(CDBBatch& batch) override;
    bool CustomAppend(const interfaces::BlockInfo& block) override;
    bool CustomRewind(const interfaces::BlockKey& current_tip, const interfaces::BlockKey& new_tip) override;
    BaseIndex::DB& GetDB() const LIFETIMEBOUND override { return *m_db; }
public:
    explicit BlockFilterIndex(std::unique_ptr<interfaces::Chain> chain, BlockFilterType filter_type,
                              size_t n_cache_size, bool f_memory = false, bool f_wipe = false);
    BlockFilterType GetFilterType() const { return m_filter_type; }
    bool LookupFilter(const CBlockIndex* block_index, BlockFilter& filter_out) const;
    bool LookupFilterHeader(const CBlockIndex* block_index, uint256& header_out) EXCLUSIVE_LOCKS_REQUIRED(!m_cs_headers_cache);
    bool LookupFilterRange(int start_height, const CBlockIndex* stop_index,
                           std::vector<BlockFilter>& filters_out) const;
    bool LookupFilterHashRange(int start_height, const CBlockIndex* stop_index,
                               std::vector<uint256>& hashes_out) const;
};
BlockFilterIndex* GetBlockFilterIndex(BlockFilterType filter_type);
void ForEachBlockFilterIndex(std::function<void (BlockFilterIndex&)> fn);
bool InitBlockFilterIndex(std::function<std::unique_ptr<interfaces::Chain>()> make_chain, BlockFilterType filter_type,
                          size_t n_cache_size, bool f_memory = false, bool f_wipe = false);
bool DestroyBlockFilterIndex(BlockFilterType filter_type);
void DestroyAllBlockFilterIndexes();
#endif
