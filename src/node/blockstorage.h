#ifndef LCRYP_NODE_BLOCKSTORAGE_H
#define LCRYP_NODE_BLOCKSTORAGE_H
#include <attributes.h>
#include <chain.h>
#include <fs.h>
#include <protocol.h>
#include <sync.h>
#include <txdb.h>
#include <atomic>
#include <cstdint>
#include <unordered_map>
#include <vector>
extern RecursiveMutex cs_main;
class ArgsManager;
class BlockValidationState;
class CBlock;
class CBlockFileInfo;
class CBlockUndo;
class CChain;
class CChainParams;
class Chainstate;
class ChainstateManager;
struct CCheckpointData;
struct FlatFilePos;
namespace Consensus {
struct Params;
}
namespace node {
static constexpr bool DEFAULT_STOPAFTERBLOCKIMPORT{false};
static const unsigned int BLOCKFILE_CHUNK_SIZE = 0x1000000;
static const unsigned int UNDOFILE_CHUNK_SIZE = 0x100000;
static const unsigned int MAX_BLOCKFILE_SIZE = 0x8000000;
extern std::atomic_bool fImporting;
extern std::atomic_bool fReindex;
extern bool fPruneMode;
extern uint64_t nPruneTarget;
using BlockMap = std::unordered_map<uint256, CBlockIndex, BlockHasher>;
struct CBlockIndexWorkComparator {
    bool operator()(const CBlockIndex* pa, const CBlockIndex* pb) const;
};
struct CBlockIndexHeightOnlyComparator {
    bool operator()(const CBlockIndex* pa, const CBlockIndex* pb) const;
};
struct PruneLockInfo {
    int height_first{std::numeric_limits<int>::max()};
};
class BlockManager
{
    friend Chainstate;
    friend ChainstateManager;
private:
    bool LoadBlockIndex(const Consensus::Params& consensus_params)
        EXCLUSIVE_LOCKS_REQUIRED(cs_main);
    void FlushBlockFile(bool fFinalize = false, bool finalize_undo = false);
    void FlushUndoFile(int block_file, bool finalize = false);
    bool FindBlockPos(FlatFilePos& pos, unsigned int nAddSize, unsigned int nHeight, CChain& active_chain, uint64_t nTime, bool fKnown);
    bool FindUndoPos(BlockValidationState& state, int nFile, FlatFilePos& pos, unsigned int nAddSize);
    void FindFilesToPruneManual(std::set<int>& setFilesToPrune, int nManualPruneHeight, int chain_tip_height);
    void FindFilesToPrune(std::set<int>& setFilesToPrune, uint64_t nPruneAfterHeight, int chain_tip_height, int prune_height, bool is_ibd);
    RecursiveMutex cs_LastBlockFile;
    std::vector<CBlockFileInfo> m_blockfile_info;
    int m_last_blockfile = 0;
    bool m_check_for_pruning = false;
    std::set<CBlockIndex*> m_dirty_blockindex;
    std::set<int> m_dirty_fileinfo;
    std::unordered_map<std::string, PruneLockInfo> m_prune_locks GUARDED_BY(::cs_main);
public:
    BlockMap m_block_index GUARDED_BY(cs_main);
    std::vector<CBlockIndex*> GetAllBlockIndices() EXCLUSIVE_LOCKS_REQUIRED(::cs_main);
    std::multimap<CBlockIndex*, CBlockIndex*> m_blocks_unlinked;
    std::unique_ptr<CBlockTreeDB> m_block_tree_db GUARDED_BY(::cs_main);
    bool WriteBlockIndexDB() EXCLUSIVE_LOCKS_REQUIRED(::cs_main);
    bool LoadBlockIndexDB(const Consensus::Params& consensus_params) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);
    CBlockIndex* AddToBlockIndex(const CBlockHeader& block, CBlockIndex*& best_header) EXCLUSIVE_LOCKS_REQUIRED(cs_main);
    CBlockIndex* InsertBlockIndex(const uint256& hash) EXCLUSIVE_LOCKS_REQUIRED(cs_main);
    void PruneOneBlockFile(const int fileNumber) EXCLUSIVE_LOCKS_REQUIRED(cs_main);
    CBlockIndex* LookupBlockIndex(const uint256& hash) EXCLUSIVE_LOCKS_REQUIRED(cs_main);
    const CBlockIndex* LookupBlockIndex(const uint256& hash) const EXCLUSIVE_LOCKS_REQUIRED(cs_main);
    CBlockFileInfo* GetBlockFileInfo(size_t n);
    bool WriteUndoDataForBlock(const CBlockUndo& blockundo, BlockValidationState& state, CBlockIndex* pindex, const CChainParams& chainparams)
        EXCLUSIVE_LOCKS_REQUIRED(::cs_main);
    FlatFilePos SaveBlockToDisk(const CBlock& block, int nHeight, CChain& active_chain, const CChainParams& chainparams, const FlatFilePos* dbp);
    uint64_t CalculateCurrentUsage();
    const CBlockIndex* GetLastCheckpoint(const CCheckpointData& data) EXCLUSIVE_LOCKS_REQUIRED(cs_main);
    const CBlockIndex* GetFirstStoredBlock(const CBlockIndex& start_block LIFETIMEBOUND) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);
    bool m_have_pruned = false;
    bool IsBlockPruned(const CBlockIndex* pblockindex) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);
    void UpdatePruneLock(const std::string& name, const PruneLockInfo& lock_info) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);
};
void CleanupBlockRevFiles();
FILE* OpenBlockFile(const FlatFilePos& pos, bool fReadOnly = false);
fs::path GetBlockPosFilename(const FlatFilePos& pos);
void UnlinkPrunedFiles(const std::set<int>& setFilesToPrune);
bool ReadBlockFromDisk(CBlock& block, const FlatFilePos& pos, const Consensus::Params& consensusParams);
bool ReadBlockFromDisk(CBlock& block, const CBlockIndex* pindex, const Consensus::Params& consensusParams);
bool ReadRawBlockFromDisk(std::vector<uint8_t>& block, const FlatFilePos& pos, const CMessageHeader::MessageStartChars& message_start);
bool UndoReadFromDisk(CBlockUndo& blockundo, const CBlockIndex* pindex);
void ThreadImport(ChainstateManager& chainman, std::vector<fs::path> vImportFiles, const ArgsManager& args, const fs::path& mempool_path);
}
#endif
