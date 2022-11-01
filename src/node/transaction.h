#ifndef LCRYP_NODE_TRANSACTION_H
#define LCRYP_NODE_TRANSACTION_H
#include <policy/feerate.h>
#include <primitives/transaction.h>
#include <util/error.h>
class CBlockIndex;
class CTxMemPool;
namespace Consensus {
struct Params;
}
namespace node {
struct NodeContext;
static const CFeeRate DEFAULT_MAX_RAW_TX_FEE_RATE{COIN / 10};
[[nodiscard]] TransactionError BroadcastTransaction(NodeContext& node, CTransactionRef tx, std::string& err_string, const CAmount& max_tx_fee, bool relay, bool wait_callback);
CTransactionRef GetTransaction(const CBlockIndex* const block_index, const CTxMemPool* const mempool, const uint256& hash, const Consensus::Params& consensusParams, uint256& hashBlock);
}
#endif
