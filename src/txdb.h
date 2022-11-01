#ifndef LCRYP_TXDB_H
#define LCRYP_TXDB_H
#include <coins.h>
#include <dbwrapper.h>
#include <sync.h>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>
class CBlockFileInfo;
class CBlockIndex;
class uint256;
namespace Consensus {
struct Params;
};
struct bilingual_str;
static const int64_t nDefaultDbCache = 450;
static const int64_t nDefaultDbBatchSize = 16 << 20;
static const int64_t nMaxDbCache = sizeof(void*) > 4 ? 16384 : 1024;
static const int64_t nMinDbCache = 4;
static const int64_t nMaxBlockDBCache = 2;
static const int64_t nMaxTxIndexCache = 1024;
static const int64_t max_filter_index_cache = 1024;
static const int64_t nMaxCoinsDBCache = 8;
extern RecursiveMutex cs_main;
class CCoinsViewDB final : public CCoinsView
{
protected:
    std::unique_ptr<CDBWrapper> m_db;
    fs::path m_ldb_path;
    bool m_is_memory;
public:
    explicit CCoinsViewDB(fs::path ldb_path, size_t nCacheSize, bool fMemory, bool fWipe);
    bool GetCoin(const COutPoint &outpoint, Coin &coin) const override;
    bool HaveCoin(const COutPoint &outpoint) const override;
    uint256 GetBestBlock() const override;
    std::vector<uint256> GetHeadBlocks() const override;
    bool BatchWrite(CCoinsMap &mapCoins, const uint256 &hashBlock) override;
    std::unique_ptr<CCoinsViewCursor> Cursor() const override;
    bool NeedsUpgrade();
    size_t EstimateSize() const override;
    void ResizeCache(size_t new_cache_size) EXCLUSIVE_LOCKS_REQUIRED(cs_main);
};
class CBlockTreeDB : public CDBWrapper
{
public:
    explicit CBlockTreeDB(size_t nCacheSize, bool fMemory = false, bool fWipe = false);
    bool WriteBatchSync(const std::vector<std::pair<int, const CBlockFileInfo*> >& fileInfo, int nLastFile, const std::vector<const CBlockIndex*>& blockinfo);
    bool ReadBlockFileInfo(int nFile, CBlockFileInfo &info);
    bool ReadLastBlockFile(int &nFile);
    bool WriteReindexing(bool fReindexing);
    void ReadReindexing(bool &fReindexing);
    bool WriteFlag(const std::string &name, bool fValue);
    bool ReadFlag(const std::string &name, bool &fValue);
    bool LoadBlockIndexGuts(const Consensus::Params& consensusParams, std::function<CBlockIndex*(const uint256&)> insertBlockIndex)
        EXCLUSIVE_LOCKS_REQUIRED(::cs_main);
};
std::optional<bilingual_str> CheckLegacyTxindex(CBlockTreeDB& block_tree_db);
#endif
