#ifndef LCRYP_POLICY_PACKAGES_H
#define LCRYP_POLICY_PACKAGES_H
#include <consensus/consensus.h>
#include <consensus/validation.h>
#include <policy/policy.h>
#include <primitives/transaction.h>
#include <cstdint>
#include <vector>
static constexpr uint32_t MAX_PACKAGE_COUNT{25};
static constexpr uint32_t MAX_PACKAGE_SIZE{101};
static_assert(MAX_PACKAGE_SIZE * WITNESS_SCALE_FACTOR * 1000 >= MAX_STANDARD_TX_WEIGHT);
static_assert(DEFAULT_DESCENDANT_LIMIT >= MAX_PACKAGE_COUNT);
static_assert(DEFAULT_ANCESTOR_LIMIT >= MAX_PACKAGE_COUNT);
static_assert(DEFAULT_ANCESTOR_SIZE_LIMIT_KVB >= MAX_PACKAGE_SIZE);
static_assert(DEFAULT_DESCENDANT_SIZE_LIMIT_KVB >= MAX_PACKAGE_SIZE);
enum class PackageValidationResult {
    PCKG_RESULT_UNSET = 0,
    PCKG_POLICY,
    PCKG_TX,
    PCKG_MEMPOOL_ERROR,
};
using Package = std::vector<CTransactionRef>;
class PackageValidationState : public ValidationState<PackageValidationResult> {};
bool CheckPackage(const Package& txns, PackageValidationState& state);
bool IsChildWithParents(const Package& package);
#endif
