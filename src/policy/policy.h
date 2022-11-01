#ifndef LCRYP_POLICY_POLICY_H
#define LCRYP_POLICY_POLICY_H
#include <consensus/amount.h>
#include <consensus/consensus.h>
#include <primitives/transaction.h>
#include <script/interpreter.h>
#include <script/standard.h>
#include <cstdint>
#include <string>
class CCoinsViewCache;
class CFeeRate;
class CScript;
static constexpr unsigned int DEFAULT_BLOCK_MAX_WEIGHT{MAX_BLOCK_WEIGHT - 4000};
static constexpr unsigned int DEFAULT_BLOCK_MIN_TX_FEE{1000};
static constexpr unsigned int MAX_STANDARD_TX_WEIGHT{400000};
static constexpr unsigned int MIN_STANDARD_TX_NONWITNESS_SIZE{82};
static constexpr unsigned int MAX_P2SH_SIGOPS{15};
static constexpr unsigned int MAX_STANDARD_TX_SIGOPS_COST{MAX_BLOCK_SIGOPS_COST/5};
static constexpr unsigned int DEFAULT_INCREMENTAL_RELAY_FEE{1000};
static constexpr unsigned int DEFAULT_BYTES_PER_SIGOP{20};
static constexpr bool DEFAULT_PERMIT_BAREMULTISIG{true};
static constexpr unsigned int MAX_STANDARD_P2WSH_STACK_ITEMS{100};
static constexpr unsigned int MAX_STANDARD_P2WSH_STACK_ITEM_SIZE{80};
static constexpr unsigned int MAX_STANDARD_TAPSCRIPT_STACK_ITEM_SIZE{80};
static constexpr unsigned int MAX_STANDARD_P2WSH_SCRIPT_SIZE{3600};
static constexpr unsigned int MAX_STANDARD_SCRIPTSIG_SIZE{1650};
static constexpr unsigned int DUST_RELAY_TX_FEE{3000};
static constexpr unsigned int DEFAULT_MIN_RELAY_TX_FEE{1000};
static constexpr unsigned int DEFAULT_ANCESTOR_LIMIT{25};
static constexpr unsigned int DEFAULT_ANCESTOR_SIZE_LIMIT_KVB{101};
static constexpr unsigned int DEFAULT_DESCENDANT_LIMIT{25};
static constexpr unsigned int DEFAULT_DESCENDANT_SIZE_LIMIT_KVB{101};
static constexpr unsigned int EXTRA_DESCENDANT_TX_SIZE_LIMIT{10000};
static constexpr unsigned int STANDARD_SCRIPT_VERIFY_FLAGS{MANDATORY_SCRIPT_VERIFY_FLAGS |
                                                             SCRIPT_VERIFY_DERSIG |
                                                             SCRIPT_VERIFY_STRICTENC |
                                                             SCRIPT_VERIFY_MINIMALDATA |
                                                             SCRIPT_VERIFY_NULLDUMMY |
                                                             SCRIPT_VERIFY_DISCOURAGE_UPGRADABLE_NOPS |
                                                             SCRIPT_VERIFY_CLEANSTACK |
                                                             SCRIPT_VERIFY_MINIMALIF |
                                                             SCRIPT_VERIFY_NULLFAIL |
                                                             SCRIPT_VERIFY_CHECKLOCKTIMEVERIFY |
                                                             SCRIPT_VERIFY_CHECKSEQUENCEVERIFY |
                                                             SCRIPT_VERIFY_LOW_S |
                                                             SCRIPT_VERIFY_WITNESS |
                                                             SCRIPT_VERIFY_DISCOURAGE_UPGRADABLE_WITNESS_PROGRAM |
                                                             SCRIPT_VERIFY_WITNESS_PUBKEYTYPE |
                                                             SCRIPT_VERIFY_CONST_SCRIPTCODE |
                                                             SCRIPT_VERIFY_TAPROOT |
                                                             SCRIPT_VERIFY_DISCOURAGE_UPGRADABLE_TAPROOT_VERSION |
                                                             SCRIPT_VERIFY_DISCOURAGE_OP_SUCCESS |
                                                             SCRIPT_VERIFY_DISCOURAGE_UPGRADABLE_PUBKEYTYPE};
static constexpr unsigned int STANDARD_NOT_MANDATORY_VERIFY_FLAGS{STANDARD_SCRIPT_VERIFY_FLAGS & ~MANDATORY_SCRIPT_VERIFY_FLAGS};
static constexpr unsigned int STANDARD_LOCKTIME_VERIFY_FLAGS{LOCKTIME_VERIFY_SEQUENCE};
CAmount GetDustThreshold(const CTxOut& txout, const CFeeRate& dustRelayFee);
bool IsDust(const CTxOut& txout, const CFeeRate& dustRelayFee);
bool IsStandard(const CScript& scriptPubKey, const std::optional<unsigned>& max_datacarrier_bytes, TxoutType& whichType);
static constexpr decltype(CTransaction::nVersion) TX_MAX_STANDARD_VERSION{2};
bool IsStandardTx(const CTransaction& tx, const std::optional<unsigned>& max_datacarrier_bytes, bool permit_bare_multisig, const CFeeRate& dust_relay_fee, std::string& reason);
bool AreInputsStandard(const CTransaction& tx, const CCoinsViewCache& mapInputs);
bool IsWitnessStandard(const CTransaction& tx, const CCoinsViewCache& mapInputs);
int64_t GetVirtualTransactionSize(int64_t nWeight, int64_t nSigOpCost, unsigned int bytes_per_sigop);
int64_t GetVirtualTransactionSize(const CTransaction& tx, int64_t nSigOpCost, unsigned int bytes_per_sigop);
int64_t GetVirtualTransactionInputSize(const CTxIn& tx, int64_t nSigOpCost, unsigned int bytes_per_sigop);
static inline int64_t GetVirtualTransactionSize(const CTransaction& tx)
{
    return GetVirtualTransactionSize(tx, 0, 0);
}
static inline int64_t GetVirtualTransactionInputSize(const CTxIn& tx)
{
    return GetVirtualTransactionInputSize(tx, 0, 0);
}
#endif
