#include <consensus/validation.h>
#include <index/txindex.h>
#include <net.h>
#include <net_processing.h>
#include <node/blockstorage.h>
#include <node/context.h>
#include <txmempool.h>
#include <validation.h>
#include <validationinterface.h>
#include <node/transaction.h>
#include <future>
namespace node {
static TransactionError HandleATMPError(const TxValidationState& state, std::string& err_string_out)
{
    err_string_out = state.ToString();
    if (state.IsInvalid()) {
        if (state.GetResult() == TxValidationResult::TX_MISSING_INPUTS) {
            return TransactionError::MISSING_INPUTS;
        }
        return TransactionError::MEMPOOL_REJECTED;
    } else {
        return TransactionError::MEMPOOL_ERROR;
    }
}
TransactionError BroadcastTransaction(NodeContext& node, const CTransactionRef tx, std::string& err_string, const CAmount& max_tx_fee, bool relay, bool wait_callback)
{
    assert(node.chainman);
    assert(node.mempool);
    assert(node.peerman);
    std::promise<void> promise;
    uint256 txid = tx->GetHash();
    uint256 wtxid = tx->GetWitnessHash();
    bool callback_set = false;
    {
        LOCK(cs_main);
        CCoinsViewCache &view = node.chainman->ActiveChainstate().CoinsTip();
        for (size_t o = 0; o < tx->vout.size(); o++) {
            const Coin& existingCoin = view.AccessCoin(COutPoint(txid, o));
            if (!existingCoin.IsSpent()) return TransactionError::ALREADY_IN_CHAIN;
        }
        if (auto mempool_tx = node.mempool->get(txid); mempool_tx) {
            wtxid = mempool_tx->GetWitnessHash();
        } else {
            if (max_tx_fee > 0) {
                const MempoolAcceptResult result = node.chainman->ProcessTransaction(tx,   true);
                if (result.m_result_type != MempoolAcceptResult::ResultType::VALID) {
                    return HandleATMPError(result.m_state, err_string);
                } else if (result.m_base_fees.value() > max_tx_fee) {
                    return TransactionError::MAX_FEE_EXCEEDED;
                }
            }
            const MempoolAcceptResult result = node.chainman->ProcessTransaction(tx,   false);
            if (result.m_result_type != MempoolAcceptResult::ResultType::VALID) {
                return HandleATMPError(result.m_state, err_string);
            }
            if (relay) {
                node.mempool->AddUnbroadcastTx(txid);
            }
            if (wait_callback) {
                CallFunctionInValidationInterfaceQueue([&promise] {
                    promise.set_value();
                });
                callback_set = true;
            }
        }
    }
    if (callback_set) {
        promise.get_future().wait();
    }
    if (relay) {
        node.peerman->RelayTransaction(txid, wtxid);
    }
    return TransactionError::OK;
}
CTransactionRef GetTransaction(const CBlockIndex* const block_index, const CTxMemPool* const mempool, const uint256& hash, const Consensus::Params& consensusParams, uint256& hashBlock)
{
    if (mempool && !block_index) {
        CTransactionRef ptx = mempool->get(hash);
        if (ptx) return ptx;
    }
    if (g_txindex) {
        CTransactionRef tx;
        uint256 block_hash;
        if (g_txindex->FindTx(hash, block_hash, tx)) {
            if (!block_index || block_index->GetBlockHash() == block_hash) {
                hashBlock = block_hash;
                return tx;
            }
        }
    }
    if (block_index) {
        CBlock block;
        if (ReadBlockFromDisk(block, block_index, consensusParams)) {
            for (const auto& tx : block.vtx) {
                if (tx->GetHash() == hash) {
                    hashBlock = block_index->GetBlockHash();
                    return tx;
                }
            }
        }
    }
    return nullptr;
}
}
