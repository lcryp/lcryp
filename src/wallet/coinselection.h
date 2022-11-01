#ifndef LCRYP_WALLET_COINSELECTION_H
#define LCRYP_WALLET_COINSELECTION_H
#include <consensus/amount.h>
#include <policy/feerate.h>
#include <primitives/transaction.h>
#include <random.h>
#include <optional>
namespace wallet {
static constexpr CAmount CHANGE_LOWER{50000};
static constexpr CAmount CHANGE_UPPER{1000000};
struct COutput {
private:
    std::optional<CAmount> effective_value;
    std::optional<CAmount> fee;
public:
    COutPoint outpoint;
    CTxOut txout;
    int depth;
    int input_bytes;
    bool spendable;
    bool solvable;
    bool safe;
    int64_t time;
    bool from_me;
    CAmount long_term_fee{0};
    COutput(const COutPoint& outpoint, const CTxOut& txout, int depth, int input_bytes, bool spendable, bool solvable, bool safe, int64_t time, bool from_me, const std::optional<CFeeRate> feerate = std::nullopt)
        : outpoint{outpoint},
          txout{txout},
          depth{depth},
          input_bytes{input_bytes},
          spendable{spendable},
          solvable{solvable},
          safe{safe},
          time{time},
          from_me{from_me}
    {
        if (feerate) {
            fee = input_bytes < 0 ? 0 : feerate.value().GetFee(input_bytes);
            effective_value = txout.nValue - fee.value();
        }
    }
    COutput(const COutPoint& outpoint, const CTxOut& txout, int depth, int input_bytes, bool spendable, bool solvable, bool safe, int64_t time, bool from_me, const CAmount fees)
        : COutput(outpoint, txout, depth, input_bytes, spendable, solvable, safe, time, from_me)
    {
        assert((input_bytes < 0 && fees == 0) || (input_bytes > 0 && fees >= 0));
        fee = fees;
        effective_value = txout.nValue - fee.value();
    }
    std::string ToString() const;
    bool operator<(const COutput& rhs) const
    {
        return outpoint < rhs.outpoint;
    }
    CAmount GetFee() const
    {
        assert(fee.has_value());
        return fee.value();
    }
    CAmount GetEffectiveValue() const
    {
        assert(effective_value.has_value());
        return effective_value.value();
    }
};
struct CoinSelectionParams {
    FastRandomContext& rng_fast;
    size_t change_output_size = 0;
    size_t change_spend_size = 0;
    CAmount m_min_change_target{0};
    CAmount min_viable_change{0};
    CAmount m_change_fee{0};
    CAmount m_cost_of_change{0};
    CFeeRate m_effective_feerate;
    CFeeRate m_long_term_feerate;
    CFeeRate m_discard_feerate;
    size_t tx_noinputs_size = 0;
    bool m_subtract_fee_outputs = false;
    bool m_avoid_partial_spends = false;
    CoinSelectionParams(FastRandomContext& rng_fast, size_t change_output_size, size_t change_spend_size,
                        CAmount min_change_target, CFeeRate effective_feerate,
                        CFeeRate long_term_feerate, CFeeRate discard_feerate, size_t tx_noinputs_size, bool avoid_partial)
        : rng_fast{rng_fast},
          change_output_size(change_output_size),
          change_spend_size(change_spend_size),
          m_min_change_target(min_change_target),
          m_effective_feerate(effective_feerate),
          m_long_term_feerate(long_term_feerate),
          m_discard_feerate(discard_feerate),
          tx_noinputs_size(tx_noinputs_size),
          m_avoid_partial_spends(avoid_partial)
    {
    }
    CoinSelectionParams(FastRandomContext& rng_fast)
        : rng_fast{rng_fast} {}
};
struct CoinEligibilityFilter
{
    const int conf_mine;
    const int conf_theirs;
    const uint64_t max_ancestors;
    const uint64_t max_descendants;
    const bool m_include_partial_groups{false};
    CoinEligibilityFilter(int conf_mine, int conf_theirs, uint64_t max_ancestors) : conf_mine(conf_mine), conf_theirs(conf_theirs), max_ancestors(max_ancestors), max_descendants(max_ancestors) {}
    CoinEligibilityFilter(int conf_mine, int conf_theirs, uint64_t max_ancestors, uint64_t max_descendants) : conf_mine(conf_mine), conf_theirs(conf_theirs), max_ancestors(max_ancestors), max_descendants(max_descendants) {}
    CoinEligibilityFilter(int conf_mine, int conf_theirs, uint64_t max_ancestors, uint64_t max_descendants, bool include_partial) : conf_mine(conf_mine), conf_theirs(conf_theirs), max_ancestors(max_ancestors), max_descendants(max_descendants), m_include_partial_groups(include_partial) {}
};
struct OutputGroup
{
    std::vector<COutput> m_outputs;
    bool m_from_me{true};
    CAmount m_value{0};
    int m_depth{999};
    size_t m_ancestors{0};
    size_t m_descendants{0};
    CAmount effective_value{0};
    CAmount fee{0};
    CFeeRate m_effective_feerate{0};
    CAmount long_term_fee{0};
    CFeeRate m_long_term_feerate{0};
    bool m_subtract_fee_outputs{false};
    OutputGroup() {}
    OutputGroup(const CoinSelectionParams& params) :
        m_effective_feerate(params.m_effective_feerate),
        m_long_term_feerate(params.m_long_term_feerate),
        m_subtract_fee_outputs(params.m_subtract_fee_outputs)
    {}
    void Insert(const COutput& output, size_t ancestors, size_t descendants, bool positive_only);
    bool EligibleForSpending(const CoinEligibilityFilter& eligibility_filter) const;
    CAmount GetSelectionAmount() const;
};
[[nodiscard]] CAmount GetSelectionWaste(const std::set<COutput>& inputs, CAmount change_cost, CAmount target, bool use_effective_value = true);
[[nodiscard]] CAmount GenerateChangeTarget(const CAmount payment_value, const CAmount change_fee, FastRandomContext& rng);
enum class SelectionAlgorithm : uint8_t
{
    BNB = 0,
    KNAPSACK = 1,
    SRD = 2,
    MANUAL = 3,
};
std::string GetAlgorithmName(const SelectionAlgorithm algo);
struct SelectionResult
{
private:
    std::set<COutput> m_selected_inputs;
    CAmount m_target;
    SelectionAlgorithm m_algo;
    bool m_use_effective{false};
    std::optional<CAmount> m_waste;
public:
    explicit SelectionResult(const CAmount target, SelectionAlgorithm algo)
        : m_target(target), m_algo(algo) {}
    SelectionResult() = delete;
    [[nodiscard]] CAmount GetSelectedValue() const;
    [[nodiscard]] CAmount GetSelectedEffectiveValue() const;
    void Clear();
    void AddInput(const OutputGroup& group);
    void ComputeAndSetWaste(const CAmount min_viable_change, const CAmount change_cost, const CAmount change_fee);
    [[nodiscard]] CAmount GetWaste() const;
    void Merge(const SelectionResult& other);
    const std::set<COutput>& GetInputSet() const;
    std::vector<COutput> GetShuffledInputVector() const;
    bool operator<(SelectionResult other) const;
    CAmount GetChange(const CAmount min_viable_change, const CAmount change_fee) const;
    CAmount GetTarget() const { return m_target; }
    SelectionAlgorithm GetAlgo() const { return m_algo; }
};
std::optional<SelectionResult> SelectCoinsBnB(std::vector<OutputGroup>& utxo_pool, const CAmount& selection_target, const CAmount& cost_of_change);
std::optional<SelectionResult> SelectCoinsSRD(const std::vector<OutputGroup>& utxo_pool, CAmount target_value, FastRandomContext& rng);
std::optional<SelectionResult> KnapsackSolver(std::vector<OutputGroup>& groups, const CAmount& nTargetValue,
                                              CAmount change_target, FastRandomContext& rng);
}
#endif
