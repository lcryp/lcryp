#ifndef LCRYP_CONSENSUS_TX_CHECK_H
#define LCRYP_CONSENSUS_TX_CHECK_H
class CTransaction;
class TxValidationState;
bool CheckTransaction(const CTransaction& tx, TxValidationState& state);
#endif
