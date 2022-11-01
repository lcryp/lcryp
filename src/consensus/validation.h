#ifndef LCRYP_CONSENSUS_VALIDATION_H
#define LCRYP_CONSENSUS_VALIDATION_H
#include <string>
#include <version.h>
#include <consensus/consensus.h>
#include <primitives/transaction.h>
#include <primitives/block.h>
static constexpr int NO_WITNESS_COMMITMENT{-1};
static constexpr size_t MINIMUM_WITNESS_COMMITMENT{38};
enum class TxValidationResult {
    TX_RESULT_UNSET = 0,
    TX_CONSENSUS,
    TX_RECENT_CONSENSUS_CHANGE,
    TX_INPUTS_NOT_STANDARD,
    TX_NOT_STANDARD,
    TX_MISSING_INPUTS,
    TX_PREMATURE_SPEND,
    TX_WITNESS_MUTATED,
    TX_WITNESS_STRIPPED,
    TX_CONFLICT,
    TX_MEMPOOL_POLICY,
    TX_NO_MEMPOOL,
};
enum class BlockValidationResult {
    BLOCK_RESULT_UNSET = 0,
    BLOCK_CONSENSUS,
    BLOCK_RECENT_CONSENSUS_CHANGE,
    BLOCK_CACHED_INVALID,
    BLOCK_INVALID_HEADER,
    BLOCK_MUTATED,
    BLOCK_MISSING_PREV,
    BLOCK_INVALID_PREV,
    BLOCK_TIME_FUTURE,
    BLOCK_CHECKPOINT,
    BLOCK_HEADER_LOW_WORK
};
template <typename Result>
class ValidationState
{
private:
    enum class ModeState {
        M_VALID,
        M_INVALID,
        M_ERROR,
    } m_mode{ModeState::M_VALID};
    Result m_result{};
    std::string m_reject_reason;
    std::string m_debug_message;
public:
    bool Invalid(Result result,
                 const std::string& reject_reason = "",
                 const std::string& debug_message = "")
    {
        m_result = result;
        m_reject_reason = reject_reason;
        m_debug_message = debug_message;
        if (m_mode != ModeState::M_ERROR) m_mode = ModeState::M_INVALID;
        return false;
    }
    bool Error(const std::string& reject_reason)
    {
        if (m_mode == ModeState::M_VALID)
            m_reject_reason = reject_reason;
        m_mode = ModeState::M_ERROR;
        return false;
    }
    bool IsValid() const { return m_mode == ModeState::M_VALID; }
    bool IsInvalid() const { return m_mode == ModeState::M_INVALID; }
    bool IsError() const { return m_mode == ModeState::M_ERROR; }
    Result GetResult() const { return m_result; }
    std::string GetRejectReason() const { return m_reject_reason; }
    std::string GetDebugMessage() const { return m_debug_message; }
    std::string ToString() const
    {
        if (IsValid()) {
            return "Valid";
        }
        if (!m_debug_message.empty()) {
            return m_reject_reason + ", " + m_debug_message;
        }
        return m_reject_reason;
    }
};
class TxValidationState : public ValidationState<TxValidationResult> {};
class BlockValidationState : public ValidationState<BlockValidationResult> {};
static inline int64_t GetTransactionWeight(const CTransaction& tx)
{
    return ::GetSerializeSize(tx, PROTOCOL_VERSION | SERIALIZE_TRANSACTION_NO_WITNESS) * (WITNESS_SCALE_FACTOR - 1) + ::GetSerializeSize(tx, PROTOCOL_VERSION);
}
static inline int64_t GetBlockWeight(const CBlock& block)
{
    return ::GetSerializeSize(block, PROTOCOL_VERSION | SERIALIZE_TRANSACTION_NO_WITNESS) * (WITNESS_SCALE_FACTOR - 1) + ::GetSerializeSize(block, PROTOCOL_VERSION);
}
static inline int64_t GetTransactionInputWeight(const CTxIn& txin)
{
    return ::GetSerializeSize(txin, PROTOCOL_VERSION | SERIALIZE_TRANSACTION_NO_WITNESS) * (WITNESS_SCALE_FACTOR - 1) + ::GetSerializeSize(txin, PROTOCOL_VERSION) + ::GetSerializeSize(txin.scriptWitness.stack, PROTOCOL_VERSION);
}
inline int GetWitnessCommitmentIndex(const CBlock& block)
{
    int commitpos = NO_WITNESS_COMMITMENT;
    if (!block.vtx.empty()) {
        for (size_t o = 0; o < block.vtx[0]->vout.size(); o++) {
            const CTxOut& vout = block.vtx[0]->vout[o];
            if (vout.scriptPubKey.size() >= MINIMUM_WITNESS_COMMITMENT &&
                vout.scriptPubKey[0] == OP_RETURN &&
                vout.scriptPubKey[1] == 0x24 &&
                vout.scriptPubKey[2] == 0xaa &&
                vout.scriptPubKey[3] == 0x21 &&
                vout.scriptPubKey[4] == 0xa9 &&
                vout.scriptPubKey[5] == 0xed) {
                commitpos = o;
            }
        }
    }
    return commitpos;
}
#endif
