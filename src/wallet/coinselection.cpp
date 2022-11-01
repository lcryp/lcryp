#include <wallet/coinselection.h>
#include <consensus/amount.h>
#include <policy/feerate.h>
#include <util/check.h>
#include <util/system.h>
#include <util/moneystr.h>
#include <numeric>
#include <optional>
namespace wallet {
struct {
    bool operator()(const OutputGroup& a, const OutputGroup& b) const
    {
        return a.GetSelectionAmount() > b.GetSelectionAmount();
    }
} descending;
static const size_t TOTAL_TRIES = 100000;
std::optional<SelectionResult> SelectCoinsBnB(std::vector<OutputGroup>& utxo_pool, const CAmount& selection_target, const CAmount& cost_of_change)
{
    SelectionResult result(selection_target, SelectionAlgorithm::BNB);
    CAmount curr_value = 0;
    std::vector<size_t> curr_selection;
    CAmount curr_available_value = 0;
    for (const OutputGroup& utxo : utxo_pool) {
        assert(utxo.GetSelectionAmount() > 0);
        curr_available_value += utxo.GetSelectionAmount();
    }
    if (curr_available_value < selection_target) {
        return std::nullopt;
    }
    std::sort(utxo_pool.begin(), utxo_pool.end(), descending);
    CAmount curr_waste = 0;
    std::vector<size_t> best_selection;
    CAmount best_waste = MAX_MONEY;
    for (size_t curr_try = 0, utxo_pool_index = 0; curr_try < TOTAL_TRIES; ++curr_try, ++utxo_pool_index) {
        bool backtrack = false;
        if (curr_value + curr_available_value < selection_target ||
            curr_value > selection_target + cost_of_change ||
            (curr_waste > best_waste && (utxo_pool.at(0).fee - utxo_pool.at(0).long_term_fee) > 0)) {
            backtrack = true;
        } else if (curr_value >= selection_target) {
            curr_waste += (curr_value - selection_target);
            if (curr_waste <= best_waste) {
                best_selection = curr_selection;
                best_waste = curr_waste;
            }
            curr_waste -= (curr_value - selection_target);
            backtrack = true;
        }
        if (backtrack) {
            if (curr_selection.empty()) {
                break;
            }
            for (--utxo_pool_index; utxo_pool_index > curr_selection.back(); --utxo_pool_index) {
                curr_available_value += utxo_pool.at(utxo_pool_index).GetSelectionAmount();
            }
            assert(utxo_pool_index == curr_selection.back());
            OutputGroup& utxo = utxo_pool.at(utxo_pool_index);
            curr_value -= utxo.GetSelectionAmount();
            curr_waste -= utxo.fee - utxo.long_term_fee;
            curr_selection.pop_back();
        } else {
            OutputGroup& utxo = utxo_pool.at(utxo_pool_index);
            curr_available_value -= utxo.GetSelectionAmount();
            if (curr_selection.empty() ||
                (utxo_pool_index - 1) == curr_selection.back() ||
                utxo.GetSelectionAmount() != utxo_pool.at(utxo_pool_index - 1).GetSelectionAmount() ||
                utxo.fee != utxo_pool.at(utxo_pool_index - 1).fee)
            {
                curr_selection.push_back(utxo_pool_index);
                curr_value += utxo.GetSelectionAmount();
                curr_waste += utxo.fee - utxo.long_term_fee;
            }
        }
    }
    if (best_selection.empty()) {
        return std::nullopt;
    }
    for (const size_t& i : best_selection) {
        result.AddInput(utxo_pool.at(i));
    }
    result.ComputeAndSetWaste(cost_of_change, cost_of_change, CAmount{0});
    assert(best_waste == result.GetWaste());
    return result;
}
std::optional<SelectionResult> SelectCoinsSRD(const std::vector<OutputGroup>& utxo_pool, CAmount target_value, FastRandomContext& rng)
{
    SelectionResult result(target_value, SelectionAlgorithm::SRD);
    target_value += CHANGE_LOWER;
    std::vector<size_t> indexes;
    indexes.resize(utxo_pool.size());
    std::iota(indexes.begin(), indexes.end(), 0);
    Shuffle(indexes.begin(), indexes.end(), rng);
    CAmount selected_eff_value = 0;
    for (const size_t i : indexes) {
        const OutputGroup& group = utxo_pool.at(i);
        Assume(group.GetSelectionAmount() > 0);
        selected_eff_value += group.GetSelectionAmount();
        result.AddInput(group);
        if (selected_eff_value >= target_value) {
            return result;
        }
    }
    return std::nullopt;
}
static void ApproximateBestSubset(FastRandomContext& insecure_rand, const std::vector<OutputGroup>& groups,
                                  const CAmount& nTotalLower, const CAmount& nTargetValue,
                                  std::vector<char>& vfBest, CAmount& nBest, int iterations = 1000)
{
    std::vector<char> vfIncluded;
    vfBest.assign(groups.size(), true);
    nBest = nTotalLower;
    for (int nRep = 0; nRep < iterations && nBest != nTargetValue; nRep++)
    {
        vfIncluded.assign(groups.size(), false);
        CAmount nTotal = 0;
        bool fReachedTarget = false;
        for (int nPass = 0; nPass < 2 && !fReachedTarget; nPass++)
        {
            for (unsigned int i = 0; i < groups.size(); i++)
            {
                if (nPass == 0 ? insecure_rand.randbool() : !vfIncluded[i])
                {
                    nTotal += groups[i].GetSelectionAmount();
                    vfIncluded[i] = true;
                    if (nTotal >= nTargetValue)
                    {
                        fReachedTarget = true;
                        if (nTotal < nBest)
                        {
                            nBest = nTotal;
                            vfBest = vfIncluded;
                        }
                        nTotal -= groups[i].GetSelectionAmount();
                        vfIncluded[i] = false;
                    }
                }
            }
        }
    }
}
std::optional<SelectionResult> KnapsackSolver(std::vector<OutputGroup>& groups, const CAmount& nTargetValue,
                                              CAmount change_target, FastRandomContext& rng)
{
    SelectionResult result(nTargetValue, SelectionAlgorithm::KNAPSACK);
    std::optional<OutputGroup> lowest_larger;
    std::vector<OutputGroup> applicable_groups;
    CAmount nTotalLower = 0;
    Shuffle(groups.begin(), groups.end(), rng);
    for (const OutputGroup& group : groups) {
        if (group.GetSelectionAmount() == nTargetValue) {
            result.AddInput(group);
            return result;
        } else if (group.GetSelectionAmount() < nTargetValue + change_target) {
            applicable_groups.push_back(group);
            nTotalLower += group.GetSelectionAmount();
        } else if (!lowest_larger || group.GetSelectionAmount() < lowest_larger->GetSelectionAmount()) {
            lowest_larger = group;
        }
    }
    if (nTotalLower == nTargetValue) {
        for (const auto& group : applicable_groups) {
            result.AddInput(group);
        }
        return result;
    }
    if (nTotalLower < nTargetValue) {
        if (!lowest_larger) return std::nullopt;
        result.AddInput(*lowest_larger);
        return result;
    }
    std::sort(applicable_groups.begin(), applicable_groups.end(), descending);
    std::vector<char> vfBest;
    CAmount nBest;
    ApproximateBestSubset(rng, applicable_groups, nTotalLower, nTargetValue, vfBest, nBest);
    if (nBest != nTargetValue && nTotalLower >= nTargetValue + change_target) {
        ApproximateBestSubset(rng, applicable_groups, nTotalLower, nTargetValue + change_target, vfBest, nBest);
    }
    if (lowest_larger &&
        ((nBest != nTargetValue && nBest < nTargetValue + change_target) || lowest_larger->GetSelectionAmount() <= nBest)) {
        result.AddInput(*lowest_larger);
    } else {
        for (unsigned int i = 0; i < applicable_groups.size(); i++) {
            if (vfBest[i]) {
                result.AddInput(applicable_groups[i]);
            }
        }
        if (LogAcceptCategory(BCLog::SELECTCOINS, BCLog::Level::Debug)) {
            std::string log_message{"Coin selection best subset: "};
            for (unsigned int i = 0; i < applicable_groups.size(); i++) {
                if (vfBest[i]) {
                    log_message += strprintf("%s ", FormatMoney(applicable_groups[i].m_value));
                }
            }
            LogPrint(BCLog::SELECTCOINS, "%stotal %s\n", log_message, FormatMoney(nBest));
        }
    }
    return result;
}
void OutputGroup::Insert(const COutput& output, size_t ancestors, size_t descendants, bool positive_only) {
    if (positive_only && output.GetEffectiveValue() <= 0) return;
    m_outputs.push_back(output);
    COutput& coin = m_outputs.back();
    fee += coin.GetFee();
    coin.long_term_fee = coin.input_bytes < 0 ? 0 : m_long_term_feerate.GetFee(coin.input_bytes);
    long_term_fee += coin.long_term_fee;
    effective_value += coin.GetEffectiveValue();
    m_from_me &= coin.from_me;
    m_value += coin.txout.nValue;
    m_depth = std::min(m_depth, coin.depth);
    m_ancestors += ancestors;
    m_descendants = std::max(m_descendants, descendants);
}
bool OutputGroup::EligibleForSpending(const CoinEligibilityFilter& eligibility_filter) const
{
    return m_depth >= (m_from_me ? eligibility_filter.conf_mine : eligibility_filter.conf_theirs)
        && m_ancestors <= eligibility_filter.max_ancestors
        && m_descendants <= eligibility_filter.max_descendants;
}
CAmount OutputGroup::GetSelectionAmount() const
{
    return m_subtract_fee_outputs ? m_value : effective_value;
}
CAmount GetSelectionWaste(const std::set<COutput>& inputs, CAmount change_cost, CAmount target, bool use_effective_value)
{
    assert(!inputs.empty());
    CAmount waste = 0;
    CAmount selected_effective_value = 0;
    for (const COutput& coin : inputs) {
        waste += coin.GetFee() - coin.long_term_fee;
        selected_effective_value += use_effective_value ? coin.GetEffectiveValue() : coin.txout.nValue;
    }
    if (change_cost) {
        assert(change_cost > 0);
        waste += change_cost;
    } else {
        assert(selected_effective_value >= target);
        waste += selected_effective_value - target;
    }
    return waste;
}
CAmount GenerateChangeTarget(const CAmount payment_value, const CAmount change_fee, FastRandomContext& rng)
{
    if (payment_value <= CHANGE_LOWER / 2) {
        return change_fee + CHANGE_LOWER;
    } else {
        const auto upper_bound = std::min(payment_value * 2, CHANGE_UPPER);
        return change_fee + rng.randrange(upper_bound - CHANGE_LOWER) + CHANGE_LOWER;
    }
}
void SelectionResult::ComputeAndSetWaste(const CAmount min_viable_change, const CAmount change_cost, const CAmount change_fee)
{
    const CAmount change = GetChange(min_viable_change, change_fee);
    if (change > 0) {
        m_waste = GetSelectionWaste(m_selected_inputs, change_cost, m_target, m_use_effective);
    } else {
        m_waste = GetSelectionWaste(m_selected_inputs, 0, m_target, m_use_effective);
    }
}
CAmount SelectionResult::GetWaste() const
{
    return *Assert(m_waste);
}
CAmount SelectionResult::GetSelectedValue() const
{
    return std::accumulate(m_selected_inputs.cbegin(), m_selected_inputs.cend(), CAmount{0}, [](CAmount sum, const auto& coin) { return sum + coin.txout.nValue; });
}
CAmount SelectionResult::GetSelectedEffectiveValue() const
{
    return std::accumulate(m_selected_inputs.cbegin(), m_selected_inputs.cend(), CAmount{0}, [](CAmount sum, const auto& coin) { return sum + coin.GetEffectiveValue(); });
}
void SelectionResult::Clear()
{
    m_selected_inputs.clear();
    m_waste.reset();
}
void SelectionResult::AddInput(const OutputGroup& group)
{
    util::insert(m_selected_inputs, group.m_outputs);
    m_use_effective = !group.m_subtract_fee_outputs;
}
void SelectionResult::Merge(const SelectionResult& other)
{
    m_target += other.m_target;
    m_use_effective |= other.m_use_effective;
    if (m_algo == SelectionAlgorithm::MANUAL) {
        m_algo = other.m_algo;
    }
    util::insert(m_selected_inputs, other.m_selected_inputs);
}
const std::set<COutput>& SelectionResult::GetInputSet() const
{
    return m_selected_inputs;
}
std::vector<COutput> SelectionResult::GetShuffledInputVector() const
{
    std::vector<COutput> coins(m_selected_inputs.begin(), m_selected_inputs.end());
    Shuffle(coins.begin(), coins.end(), FastRandomContext());
    return coins;
}
bool SelectionResult::operator<(SelectionResult other) const
{
    Assert(m_waste.has_value());
    Assert(other.m_waste.has_value());
    return *m_waste < *other.m_waste || (*m_waste == *other.m_waste && m_selected_inputs.size() > other.m_selected_inputs.size());
}
std::string COutput::ToString() const
{
    return strprintf("COutput(%s, %d, %d) [%s]", outpoint.hash.ToString(), outpoint.n, depth, FormatMoney(txout.nValue));
}
std::string GetAlgorithmName(const SelectionAlgorithm algo)
{
    switch (algo)
    {
    case SelectionAlgorithm::BNB: return "bnb";
    case SelectionAlgorithm::KNAPSACK: return "knapsack";
    case SelectionAlgorithm::SRD: return "srd";
    case SelectionAlgorithm::MANUAL: return "manual";
    }
    assert(false);
}
CAmount SelectionResult::GetChange(const CAmount min_viable_change, const CAmount change_fee) const
{
    const CAmount change = m_use_effective
                           ? GetSelectedEffectiveValue() - m_target - change_fee
                           : GetSelectedValue() - m_target;
    if (change < min_viable_change) {
        return 0;
    }
    return change;
}
}
