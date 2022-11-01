#ifndef LCRYP_RPC_BLOCKCHAIN_H
#define LCRYP_RPC_BLOCKCHAIN_H
#include <consensus/amount.h>
#include <core_io.h>
#include <fs.h>
#include <streams.h>
#include <sync.h>
#include <validation.h>
#include <any>
#include <stdint.h>
#include <vector>
extern RecursiveMutex cs_main;
class CBlock;
class CBlockIndex;
class Chainstate;
class UniValue;
namespace node {
struct NodeContext;
}
static constexpr int NUM_GETBLOCKSTATS_PERCENTILES = 5;
double GetDifficulty(const CBlockIndex* blockindex);
void RPCNotifyBlockChange(const CBlockIndex*);
UniValue blockToJSON(node::BlockManager& blockman, const CBlock& block, const CBlockIndex* tip, const CBlockIndex* blockindex, TxVerbosity verbosity) LOCKS_EXCLUDED(cs_main);
UniValue blockheaderToJSON(const CBlockIndex* tip, const CBlockIndex* blockindex) LOCKS_EXCLUDED(cs_main);
void CalculatePercentilesByWeight(CAmount result[NUM_GETBLOCKSTATS_PERCENTILES], std::vector<std::pair<CAmount, int64_t>>& scores, int64_t total_weight);
UniValue CreateUTXOSnapshot(
    node::NodeContext& node,
    Chainstate& chainstate,
    AutoFile& afile,
    const fs::path& path,
    const fs::path& tmppath);
#endif
