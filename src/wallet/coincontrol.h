#ifndef LCRYP_WALLET_COINCONTROL_H
#define LCRYP_WALLET_COINCONTROL_H
#include <outputtype.h>
#include <policy/feerate.h>
#include <policy/fees.h>
#include <primitives/transaction.h>
#include <script/keyorigin.h>
#include <script/signingprovider.h>
#include <script/standard.h>
#include <optional>
#include <algorithm>
#include <map>
#include <set>
namespace wallet {
const int DEFAULT_MIN_DEPTH = 0;
const int DEFAULT_MAX_DEPTH = 9999999;
static constexpr bool DEFAULT_AVOIDPARTIALSPENDS = false;
class CCoinControl
{
public:
    CTxDestination destChange = CNoDestination();
    std::optional<OutputType> m_change_type;
    bool m_include_unsafe_inputs = false;
    bool m_allow_other_inputs = false;
    bool fAllowWatchOnly = false;
    bool fOverrideFeeRate = false;
    std::optional<CFeeRate> m_feerate;
    std::optional<unsigned int> m_confirm_target;
    std::optional<bool> m_signal_bip125_rbf;
    bool m_avoid_partial_spends = DEFAULT_AVOIDPARTIALSPENDS;
    bool m_avoid_address_reuse = false;
    FeeEstimateMode m_fee_mode = FeeEstimateMode::UNSET;
    int m_min_depth = DEFAULT_MIN_DEPTH;
    int m_max_depth = DEFAULT_MAX_DEPTH;
    FlatSigningProvider m_external_provider;
    CCoinControl();
    bool HasSelected() const
    {
        return (setSelected.size() > 0);
    }
    bool IsSelected(const COutPoint& output) const
    {
        return (setSelected.count(output) > 0);
    }
    bool IsExternalSelected(const COutPoint& output) const
    {
        return (m_external_txouts.count(output) > 0);
    }
    bool GetExternalOutput(const COutPoint& outpoint, CTxOut& txout) const
    {
        const auto ext_it = m_external_txouts.find(outpoint);
        if (ext_it == m_external_txouts.end()) {
            return false;
        }
        txout = ext_it->second;
        return true;
    }
    void Select(const COutPoint& output)
    {
        setSelected.insert(output);
    }
    void SelectExternal(const COutPoint& outpoint, const CTxOut& txout)
    {
        setSelected.insert(outpoint);
        m_external_txouts.emplace(outpoint, txout);
    }
    void UnSelect(const COutPoint& output)
    {
        setSelected.erase(output);
    }
    void UnSelectAll()
    {
        setSelected.clear();
    }
    void ListSelected(std::vector<COutPoint>& vOutpoints) const
    {
        vOutpoints.assign(setSelected.begin(), setSelected.end());
    }
    void SetInputWeight(const COutPoint& outpoint, int64_t weight)
    {
        m_input_weights[outpoint] = weight;
    }
    bool HasInputWeight(const COutPoint& outpoint) const
    {
        return m_input_weights.count(outpoint) > 0;
    }
    int64_t GetInputWeight(const COutPoint& outpoint) const
    {
        auto it = m_input_weights.find(outpoint);
        assert(it != m_input_weights.end());
        return it->second;
    }
private:
    std::set<COutPoint> setSelected;
    std::map<COutPoint, CTxOut> m_external_txouts;
    std::map<COutPoint, int64_t> m_input_weights;
};
}
#endif
