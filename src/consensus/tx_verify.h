#ifndef LCRYP_CONSENSUS_TX_VERIFY_H
#define LCRYP_CONSENSUS_TX_VERIFY_H
#include <consensus/amount.h>
#include <stdint.h>
#include <vector>
class CBlockIndex;
class CCoinsViewCache;
class CTransaction;
class TxValidationState;
namespace Consensus {
[[nodiscard]] bool CheckTxInputs(const CTransaction& tx, TxValidationState& state, const CCoinsViewCache& inputs, int nSpendHeight, CAmount& txfee);
}
unsigned int GetLegacySigOpCount(const CTransaction& tx);
unsigned int GetP2SHSigOpCount(const CTransaction& tx, const CCoinsViewCache& mapInputs);
int64_t GetTransactionSigOpCost(const CTransaction& tx, const CCoinsViewCache& inputs, uint32_t flags);
bool IsFinalTx(const CTransaction &tx, int nBlockHeight, int64_t nBlockTime);
std::pair<int, int64_t> CalculateSequenceLocks(const CTransaction &tx, int flags, std::vector<int>& prevHeights, const CBlockIndex& block);
bool EvaluateSequenceLocks(const CBlockIndex& block, std::pair<int, int64_t> lockPair);
bool SequenceLocks(const CTransaction &tx, int flags, std::vector<int>& prevHeights, const CBlockIndex& block);
#endif
