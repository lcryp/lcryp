#ifndef LCRYP_POLICY_RBF_H
#define LCRYP_POLICY_RBF_H
#include <consensus/amount.h>
#include <primitives/transaction.h>
#include <threadsafety.h>
#include <txmempool.h>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <set>
#include <string>
class CFeeRate;
class uint256;
static constexpr uint32_t MAX_REPLACEMENT_CANDIDATES{100};
enum class RBFTransactionState {
    UNKNOWN,
    REPLACEABLE_BIP125,
    FINAL,
};
RBFTransactionState IsRBFOptIn(const CTransaction& tx, const CTxMemPool& pool) EXCLUSIVE_LOCKS_REQUIRED(pool.cs);
RBFTransactionState IsRBFOptInEmptyMempool(const CTransaction& tx);
std::optional<std::string> GetEntriesForConflicts(const CTransaction& tx, CTxMemPool& pool,
                                                  const CTxMemPool::setEntries& iters_conflicting,
                                                  CTxMemPool::setEntries& all_conflicts)
    EXCLUSIVE_LOCKS_REQUIRED(pool.cs);
std::optional<std::string> HasNoNewUnconfirmed(const CTransaction& tx, const CTxMemPool& pool,
                                               const CTxMemPool::setEntries& iters_conflicting)
    EXCLUSIVE_LOCKS_REQUIRED(pool.cs);
std::optional<std::string> EntriesAndTxidsDisjoint(const CTxMemPool::setEntries& ancestors,
                                                   const std::set<uint256>& direct_conflicts,
                                                   const uint256& txid);
std::optional<std::string> PaysMoreThanConflicts(const CTxMemPool::setEntries& iters_conflicting,
                                                 CFeeRate replacement_feerate, const uint256& txid);
std::optional<std::string> PaysForRBF(CAmount original_fees,
                                      CAmount replacement_fees,
                                      size_t replacement_vsize,
                                      CFeeRate relay_fee,
                                      const uint256& txid);
#endif
