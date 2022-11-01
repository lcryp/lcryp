#include <consensus/amount.h>
#include <consensus/validation.h>
#include <interfaces/chain.h>
#include <policy/policy.h>
#include <script/signingprovider.h>
#include <util/check.h>
#include <util/fees.h>
#include <util/moneystr.h>
#include <util/rbf.h>
#include <util/trace.h>
#include <util/translation.h>
#include <wallet/coincontrol.h>
#include <wallet/fees.h>
#include <wallet/receive.h>
#include <wallet/spend.h>
#include <wallet/transaction.h>
#include <wallet/wallet.h>
#include <cmath>
using interfaces::FoundBlock;
namespace wallet {
static constexpr size_t OUTPUT_GROUP_MAX_ENTRIES{100};
int CalculateMaximumSignedInputSize(const CTxOut& txout, const COutPoint outpoint, const SigningProvider* provider, const CCoinControl* coin_control)
{
    CMutableTransaction txn;
    txn.vin.push_back(CTxIn(outpoint));
    if (!provider || !DummySignInput(*provider, txn.vin[0], txout, coin_control)) {
        return -1;
    }
    return GetVirtualTransactionInputSize(txn.vin[0]);
}
int CalculateMaximumSignedInputSize(const CTxOut& txout, const CWallet* wallet, const CCoinControl* coin_control)
{
    const std::unique_ptr<SigningProvider> provider = wallet->GetSolvingProvider(txout.scriptPubKey);
    return CalculateMaximumSignedInputSize(txout, COutPoint(), provider.get(), coin_control);
}
TxSize CalculateMaximumSignedTxSize(const CTransaction &tx, const CWallet *wallet, const std::vector<CTxOut>& txouts, const CCoinControl* coin_control)
{
    CMutableTransaction txNew(tx);
    if (!wallet->DummySignTx(txNew, txouts, coin_control)) {
        return TxSize{-1, -1};
    }
    CTransaction ctx(txNew);
    int64_t vsize = GetVirtualTransactionSize(ctx);
    int64_t weight = GetTransactionWeight(ctx);
    return TxSize{vsize, weight};
}
TxSize CalculateMaximumSignedTxSize(const CTransaction &tx, const CWallet *wallet, const CCoinControl* coin_control)
{
    std::vector<CTxOut> txouts;
    for (const CTxIn& input : tx.vin) {
        const auto mi = wallet->mapWallet.find(input.prevout.hash);
        if (mi != wallet->mapWallet.end()) {
            assert(input.prevout.n < mi->second.tx->vout.size());
            txouts.emplace_back(mi->second.tx->vout.at(input.prevout.n));
        } else if (coin_control) {
            CTxOut txout;
            if (!coin_control->GetExternalOutput(input.prevout, txout)) {
                return TxSize{-1, -1};
            }
            txouts.emplace_back(txout);
        } else {
            return TxSize{-1, -1};
        }
    }
    return CalculateMaximumSignedTxSize(tx, wallet, txouts, coin_control);
}
size_t CoinsResult::Size() const
{
    size_t size{0};
    for (const auto& it : coins) {
        size += it.second.size();
    }
    return size;
}
std::vector<COutput> CoinsResult::All() const
{
    std::vector<COutput> all;
    all.reserve(coins.size());
    for (const auto& it : coins) {
        all.insert(all.end(), it.second.begin(), it.second.end());
    }
    return all;
}
void CoinsResult::Clear() {
    coins.clear();
}
void CoinsResult::Erase(std::set<COutPoint>& preset_coins)
{
    for (auto& it : coins) {
        auto& vec = it.second;
        auto i = std::find_if(vec.begin(), vec.end(), [&](const COutput &c) { return preset_coins.count(c.outpoint);});
        if (i != vec.end()) {
            vec.erase(i);
            break;
        }
    }
}
void CoinsResult::Shuffle(FastRandomContext& rng_fast)
{
    for (auto& it : coins) {
        ::Shuffle(it.second.begin(), it.second.end(), rng_fast);
    }
}
void CoinsResult::Add(OutputType type, const COutput& out)
{
    coins[type].emplace_back(out);
}
static OutputType GetOutputType(TxoutType type, bool is_from_p2sh)
{
    switch (type) {
        case TxoutType::WITNESS_V1_TAPROOT:
            return OutputType::BECH32M;
        case TxoutType::WITNESS_V0_KEYHASH:
        case TxoutType::WITNESS_V0_SCRIPTHASH:
            if (is_from_p2sh) return OutputType::P2SH_SEGWIT;
            else return OutputType::BECH32;
        case TxoutType::SCRIPTHASH:
        case TxoutType::PUBKEYHASH:
            return OutputType::LEGACY;
        default:
            return OutputType::UNKNOWN;
    }
}
CoinsResult AvailableCoins(const CWallet& wallet,
                           const CCoinControl* coinControl,
                           std::optional<CFeeRate> feerate,
                           const CAmount& nMinimumAmount,
                           const CAmount& nMaximumAmount,
                           const CAmount& nMinimumSumAmount,
                           const uint64_t nMaximumCount,
                           bool only_spendable)
{
    AssertLockHeld(wallet.cs_wallet);
    CoinsResult result;
    bool allow_used_addresses = !wallet.IsWalletFlagSet(WALLET_FLAG_AVOID_REUSE) || (coinControl && !coinControl->m_avoid_address_reuse);
    const int min_depth = {coinControl ? coinControl->m_min_depth : DEFAULT_MIN_DEPTH};
    const int max_depth = {coinControl ? coinControl->m_max_depth : DEFAULT_MAX_DEPTH};
    const bool only_safe = {coinControl ? !coinControl->m_include_unsafe_inputs : true};
    std::set<uint256> trusted_parents;
    for (const auto& entry : wallet.mapWallet)
    {
        const uint256& wtxid = entry.first;
        const CWalletTx& wtx = entry.second;
        if (wallet.IsTxImmatureCoinBase(wtx))
            continue;
        int nDepth = wallet.GetTxDepthInMainChain(wtx);
        if (nDepth < 0)
            continue;
        if (nDepth == 0 && !wtx.InMempool())
            continue;
        bool safeTx = CachedTxIsTrusted(wallet, wtx, trusted_parents);
        if (nDepth == 0 && wtx.mapValue.count("replaces_txid")) {
            safeTx = false;
        }
        if (nDepth == 0 && wtx.mapValue.count("replaced_by_txid")) {
            safeTx = false;
        }
        if (only_safe && !safeTx) {
            continue;
        }
        if (nDepth < min_depth || nDepth > max_depth) {
            continue;
        }
        bool tx_from_me = CachedTxIsFromMe(wallet, wtx, ISMINE_ALL);
        for (unsigned int i = 0; i < wtx.tx->vout.size(); i++) {
            const CTxOut& output = wtx.tx->vout[i];
            const COutPoint outpoint(wtxid, i);
            if (output.nValue < nMinimumAmount || output.nValue > nMaximumAmount)
                continue;
            if (coinControl && coinControl->HasSelected() && !coinControl->m_allow_other_inputs && !coinControl->IsSelected(outpoint))
                continue;
            if (wallet.IsLockedCoin(outpoint))
                continue;
            if (wallet.IsSpent(outpoint))
                continue;
            isminetype mine = wallet.IsMine(output);
            if (mine == ISMINE_NO) {
                continue;
            }
            if (!allow_used_addresses && wallet.IsSpentKey(output.scriptPubKey)) {
                continue;
            }
            std::unique_ptr<SigningProvider> provider = wallet.GetSolvingProvider(output.scriptPubKey);
            int input_bytes = CalculateMaximumSignedInputSize(output, COutPoint(), provider.get(), coinControl);
            bool solvable = input_bytes > -1;
            bool spendable = ((mine & ISMINE_SPENDABLE) != ISMINE_NO) || (((mine & ISMINE_WATCH_ONLY) != ISMINE_NO) && (coinControl && coinControl->fAllowWatchOnly && solvable));
            if (!spendable && only_spendable) continue;
            std::vector<std::vector<uint8_t>> script_solutions;
            TxoutType type = Solver(output.scriptPubKey, script_solutions);
            bool is_from_p2sh{false};
            if (type == TxoutType::SCRIPTHASH && solvable) {
                CScript script;
                if (!provider->GetCScript(CScriptID(uint160(script_solutions[0])), script)) continue;
                type = Solver(script, script_solutions);
                is_from_p2sh = true;
            }
            result.Add(GetOutputType(type, is_from_p2sh),
                       COutput(outpoint, output, nDepth, input_bytes, spendable, solvable, safeTx, wtx.GetTxTime(), tx_from_me, feerate));
            result.total_amount += output.nValue;
            if (nMinimumSumAmount != MAX_MONEY) {
                if (result.total_amount >= nMinimumSumAmount) {
                    return result;
                }
            }
            if (nMaximumCount > 0 && result.Size() >= nMaximumCount) {
                return result;
            }
        }
    }
    return result;
}
CoinsResult AvailableCoinsListUnspent(const CWallet& wallet, const CCoinControl* coinControl, const CAmount& nMinimumAmount, const CAmount& nMaximumAmount, const CAmount& nMinimumSumAmount, const uint64_t nMaximumCount)
{
    return AvailableCoins(wallet, coinControl,   std::nullopt, nMinimumAmount, nMaximumAmount, nMinimumSumAmount, nMaximumCount,  false);
}
CAmount GetAvailableBalance(const CWallet& wallet, const CCoinControl* coinControl)
{
    LOCK(wallet.cs_wallet);
    return AvailableCoins(wallet, coinControl,
              std::nullopt,
              1,
              MAX_MONEY,
              MAX_MONEY,
              0
    ).total_amount;
}
const CTxOut& FindNonChangeParentOutput(const CWallet& wallet, const CTransaction& tx, int output)
{
    AssertLockHeld(wallet.cs_wallet);
    const CTransaction* ptx = &tx;
    int n = output;
    while (OutputIsChange(wallet, ptx->vout[n]) && ptx->vin.size() > 0) {
        const COutPoint& prevout = ptx->vin[0].prevout;
        auto it = wallet.mapWallet.find(prevout.hash);
        if (it == wallet.mapWallet.end() || it->second.tx->vout.size() <= prevout.n ||
            !wallet.IsMine(it->second.tx->vout[prevout.n])) {
            break;
        }
        ptx = it->second.tx.get();
        n = prevout.n;
    }
    return ptx->vout[n];
}
const CTxOut& FindNonChangeParentOutput(const CWallet& wallet, const COutPoint& outpoint)
{
    AssertLockHeld(wallet.cs_wallet);
    return FindNonChangeParentOutput(wallet, *wallet.GetWalletTx(outpoint.hash)->tx, outpoint.n);
}
std::map<CTxDestination, std::vector<COutput>> ListCoins(const CWallet& wallet)
{
    AssertLockHeld(wallet.cs_wallet);
    std::map<CTxDestination, std::vector<COutput>> result;
    for (COutput& coin : AvailableCoinsListUnspent(wallet).All()) {
        CTxDestination address;
        if ((coin.spendable || (wallet.IsWalletFlagSet(WALLET_FLAG_DISABLE_PRIVATE_KEYS) && coin.solvable)) &&
            ExtractDestination(FindNonChangeParentOutput(wallet, coin.outpoint).scriptPubKey, address)) {
            result[address].emplace_back(std::move(coin));
        }
    }
    std::vector<COutPoint> lockedCoins;
    wallet.ListLockedCoins(lockedCoins);
    const bool include_watch_only = wallet.GetLegacyScriptPubKeyMan() && wallet.IsWalletFlagSet(WALLET_FLAG_DISABLE_PRIVATE_KEYS);
    const isminetype is_mine_filter = include_watch_only ? ISMINE_WATCH_ONLY : ISMINE_SPENDABLE;
    for (const COutPoint& output : lockedCoins) {
        auto it = wallet.mapWallet.find(output.hash);
        if (it != wallet.mapWallet.end()) {
            const auto& wtx = it->second;
            int depth = wallet.GetTxDepthInMainChain(wtx);
            if (depth >= 0 && output.n < wtx.tx->vout.size() &&
                wallet.IsMine(wtx.tx->vout[output.n]) == is_mine_filter
            ) {
                CTxDestination address;
                if (ExtractDestination(FindNonChangeParentOutput(wallet, *wtx.tx, output.n).scriptPubKey, address)) {
                    const auto out = wtx.tx->vout.at(output.n);
                    result[address].emplace_back(
                            COutPoint(wtx.GetHash(), output.n), out, depth, CalculateMaximumSignedInputSize(out, &wallet,  nullptr),   true,   true,   false, wtx.GetTxTime(), CachedTxIsFromMe(wallet, wtx, ISMINE_ALL));
                }
            }
        }
    }
    return result;
}
std::vector<OutputGroup> GroupOutputs(const CWallet& wallet, const std::vector<COutput>& outputs, const CoinSelectionParams& coin_sel_params, const CoinEligibilityFilter& filter, bool positive_only)
{
    std::vector<OutputGroup> groups_out;
    if (!coin_sel_params.m_avoid_partial_spends) {
        for (const COutput& output : outputs) {
            if (!output.spendable) continue;
            size_t ancestors, descendants;
            wallet.chain().getTransactionAncestry(output.outpoint.hash, ancestors, descendants);
            OutputGroup group{coin_sel_params};
            group.Insert(output, ancestors, descendants, positive_only);
            if (positive_only && group.GetSelectionAmount() <= 0) continue;
            if (group.m_outputs.size() > 0 && group.EligibleForSpending(filter)) groups_out.push_back(group);
        }
        return groups_out;
    }
    std::map<CScript, std::vector<OutputGroup>> spk_to_groups_map;
    for (const auto& output : outputs) {
        if (!output.spendable) continue;
        size_t ancestors, descendants;
        wallet.chain().getTransactionAncestry(output.outpoint.hash, ancestors, descendants);
        CScript spk = output.txout.scriptPubKey;
        std::vector<OutputGroup>& groups = spk_to_groups_map[spk];
        if (groups.size() == 0) {
            groups.emplace_back(coin_sel_params);
        }
        OutputGroup* group = &groups.back();
        if (group->m_outputs.size() >= OUTPUT_GROUP_MAX_ENTRIES) {
            groups.emplace_back(coin_sel_params);
            group = &groups.back();
        }
        group->Insert(output, ancestors, descendants, positive_only);
    }
    for (const auto& spk_and_groups_pair: spk_to_groups_map) {
        const std::vector<OutputGroup>& groups_per_spk= spk_and_groups_pair.second;
        for (auto group_it = groups_per_spk.rbegin(); group_it != groups_per_spk.rend(); group_it++) {
            const OutputGroup& group = *group_it;
            if (group_it == groups_per_spk.rbegin() && groups_per_spk.size() > 1 && !filter.m_include_partial_groups) {
                continue;
            }
            if (positive_only && group.GetSelectionAmount() <= 0) continue;
            if (group.m_outputs.size() > 0 && group.EligibleForSpending(filter)) groups_out.push_back(group);
        }
    }
    return groups_out;
}
std::optional<SelectionResult> AttemptSelection(const CWallet& wallet, const CAmount& nTargetValue, const CoinEligibilityFilter& eligibility_filter, const CoinsResult& available_coins,
                               const CoinSelectionParams& coin_selection_params, bool allow_mixed_output_types)
{
    std::vector<SelectionResult> results;
    for (const auto& it : available_coins.coins) {
        if (auto result{ChooseSelectionResult(wallet, nTargetValue, eligibility_filter, it.second, coin_selection_params)}) {
            results.push_back(*result);
        }
    }
    if (results.size() > 0) return *std::min_element(results.begin(), results.end());
    if (allow_mixed_output_types) {
        if (auto result{ChooseSelectionResult(wallet, nTargetValue, eligibility_filter, available_coins.All(), coin_selection_params)}) {
            return result;
        }
    }
    return std::nullopt;
};
std::optional<SelectionResult> ChooseSelectionResult(const CWallet& wallet, const CAmount& nTargetValue, const CoinEligibilityFilter& eligibility_filter, const std::vector<COutput>& available_coins, const CoinSelectionParams& coin_selection_params)
{
    std::vector<SelectionResult> results;
    std::vector<OutputGroup> positive_groups = GroupOutputs(wallet, available_coins, coin_selection_params, eligibility_filter,  true);
    if (auto bnb_result{SelectCoinsBnB(positive_groups, nTargetValue, coin_selection_params.m_cost_of_change)}) {
        results.push_back(*bnb_result);
    }
    std::vector<OutputGroup> all_groups = GroupOutputs(wallet, available_coins, coin_selection_params, eligibility_filter,  false);
    if (auto knapsack_result{KnapsackSolver(all_groups, nTargetValue, coin_selection_params.m_min_change_target, coin_selection_params.rng_fast)}) {
        knapsack_result->ComputeAndSetWaste(coin_selection_params.min_viable_change, coin_selection_params.m_cost_of_change, coin_selection_params.m_change_fee);
        results.push_back(*knapsack_result);
    }
    if (auto srd_result{SelectCoinsSRD(positive_groups, nTargetValue, coin_selection_params.rng_fast)}) {
        srd_result->ComputeAndSetWaste(coin_selection_params.min_viable_change, coin_selection_params.m_cost_of_change, coin_selection_params.m_change_fee);
        results.push_back(*srd_result);
    }
    if (results.size() == 0) {
        return std::nullopt;
    }
    auto& best_result = *std::min_element(results.begin(), results.end());
    return best_result;
}
std::optional<SelectionResult> SelectCoins(const CWallet& wallet, CoinsResult& available_coins, const CAmount& nTargetValue, const CCoinControl& coin_control, const CoinSelectionParams& coin_selection_params)
{
    CAmount value_to_select = nTargetValue;
    OutputGroup preset_inputs(coin_selection_params);
    std::set<COutPoint> preset_coins;
    std::vector<COutPoint> vPresetInputs;
    coin_control.ListSelected(vPresetInputs);
    for (const COutPoint& outpoint : vPresetInputs) {
        int input_bytes = -1;
        CTxOut txout;
        auto ptr_wtx = wallet.GetWalletTx(outpoint.hash);
        if (ptr_wtx) {
            if (ptr_wtx->tx->vout.size() <= outpoint.n) {
                return std::nullopt;
            }
            txout = ptr_wtx->tx->vout.at(outpoint.n);
            input_bytes = CalculateMaximumSignedInputSize(txout, &wallet, &coin_control);
        } else {
            if (!coin_control.GetExternalOutput(outpoint, txout)) {
                return std::nullopt;
            }
        }
        if (input_bytes == -1) {
            input_bytes = CalculateMaximumSignedInputSize(txout, outpoint, &coin_control.m_external_provider, &coin_control);
        }
        if (coin_control.HasInputWeight(outpoint)) {
            input_bytes = GetVirtualTransactionSize(coin_control.GetInputWeight(outpoint), 0, 0);
        }
        if (input_bytes == -1) {
            return std::nullopt;
        }
        COutput output(outpoint, txout,   0, input_bytes,   true,   true,   true,   0,   false, coin_selection_params.m_effective_feerate);
        if (coin_selection_params.m_subtract_fee_outputs) {
            value_to_select -= output.txout.nValue;
        } else {
            value_to_select -= output.GetEffectiveValue();
        }
        preset_coins.insert(outpoint);
        preset_inputs.Insert(output,   0,   0,   false);
    }
    if (coin_control.HasSelected() && !coin_control.m_allow_other_inputs) {
        SelectionResult result(nTargetValue, SelectionAlgorithm::MANUAL);
        result.AddInput(preset_inputs);
        if (result.GetSelectedValue() < nTargetValue) return std::nullopt;
        result.ComputeAndSetWaste(coin_selection_params.min_viable_change, coin_selection_params.m_cost_of_change, coin_selection_params.m_change_fee);
        return result;
    }
    if (coin_control.HasSelected()) {
        available_coins.Erase(preset_coins);
    }
    unsigned int limit_ancestor_count = 0;
    unsigned int limit_descendant_count = 0;
    wallet.chain().getPackageLimits(limit_ancestor_count, limit_descendant_count);
    const size_t max_ancestors = (size_t)std::max<int64_t>(1, limit_ancestor_count);
    const size_t max_descendants = (size_t)std::max<int64_t>(1, limit_descendant_count);
    const bool fRejectLongChains = gArgs.GetBoolArg("-walletrejectlongchains", DEFAULT_WALLET_REJECT_LONG_CHAINS);
    if (coin_control.m_avoid_partial_spends && available_coins.Size() > OUTPUT_GROUP_MAX_ENTRIES) {
        available_coins.Shuffle(coin_selection_params.rng_fast);
    }
    SelectionResult preselected(preset_inputs.GetSelectionAmount(), SelectionAlgorithm::MANUAL);
    preselected.AddInput(preset_inputs);
    std::optional<SelectionResult> res = [&] {
        if (value_to_select <= 0) return std::make_optional(SelectionResult(value_to_select, SelectionAlgorithm::MANUAL));
        if (auto r1{AttemptSelection(wallet, value_to_select, CoinEligibilityFilter(1, 6, 0), available_coins, coin_selection_params,  false)}) return r1;
        if (auto r2{AttemptSelection(wallet, value_to_select, CoinEligibilityFilter(1, 1, 0), available_coins, coin_selection_params,  true)}) return r2;
        if (wallet.m_spend_zero_conf_change) {
            if (auto r3{AttemptSelection(wallet, value_to_select, CoinEligibilityFilter(0, 1, 2), available_coins, coin_selection_params,  true)}) return r3;
            if (auto r4{AttemptSelection(wallet, value_to_select, CoinEligibilityFilter(0, 1, std::min((size_t)4, max_ancestors/3), std::min((size_t)4, max_descendants/3)),
                                   available_coins, coin_selection_params,  true)}) {
                return r4;
            }
            if (auto r5{AttemptSelection(wallet, value_to_select, CoinEligibilityFilter(0, 1, max_ancestors/2, max_descendants/2),
                                   available_coins, coin_selection_params,  true)}) {
                return r5;
            }
            if (auto r6{AttemptSelection(wallet, value_to_select, CoinEligibilityFilter(0, 1, max_ancestors-1, max_descendants-1, true  ),
                                   available_coins, coin_selection_params,  true)}) {
                return r6;
            }
            if (coin_control.m_include_unsafe_inputs) {
                if (auto r7{AttemptSelection(wallet, value_to_select,
                    CoinEligibilityFilter(0  , 0  , max_ancestors-1, max_descendants-1, true  ),
                    available_coins, coin_selection_params,  true)}) {
                    return r7;
                }
            }
            if (!fRejectLongChains) {
                if (auto r8{AttemptSelection(wallet, value_to_select,
                                      CoinEligibilityFilter(0, 1, std::numeric_limits<uint64_t>::max(), std::numeric_limits<uint64_t>::max(), true  ),
                                      available_coins, coin_selection_params,  true)}) {
                    return r8;
                }
            }
        }
        return std::optional<SelectionResult>();
    }();
    if (!res) return std::nullopt;
    res->Merge(preselected);
    if (res->GetAlgo() == SelectionAlgorithm::MANUAL) {
        res->ComputeAndSetWaste(coin_selection_params.min_viable_change, coin_selection_params.m_cost_of_change, coin_selection_params.m_change_fee);
    }
    return res;
}
static bool IsCurrentForAntiFeeSniping(interfaces::Chain& chain, const uint256& block_hash)
{
    if (chain.isInitialBlockDownload()) {
        return false;
    }
    constexpr int64_t MAX_ANTI_FEE_SNIPING_TIP_AGE = 8 * 60 * 60;
    int64_t block_time;
    CHECK_NONFATAL(chain.findBlock(block_hash, FoundBlock().time(block_time)));
    if (block_time < (GetTime() - MAX_ANTI_FEE_SNIPING_TIP_AGE)) {
        return false;
    }
    return true;
}
static void DiscourageFeeSniping(CMutableTransaction& tx, FastRandomContext& rng_fast,
                                 interfaces::Chain& chain, const uint256& block_hash, int block_height)
{
    assert(!tx.vin.empty());
    if (IsCurrentForAntiFeeSniping(chain, block_hash)) {
        tx.nLockTime = block_height;
        if (rng_fast.randrange(10) == 0) {
            tx.nLockTime = std::max(0, int(tx.nLockTime) - int(rng_fast.randrange(100)));
        }
    } else {
        tx.nLockTime = 0;
    }
    assert(tx.nLockTime < LOCKTIME_THRESHOLD);
    assert(tx.nLockTime <= uint64_t(block_height));
    for (const auto& in : tx.vin) {
        assert(in.nSequence != CTxIn::SEQUENCE_FINAL);
        if (in.nSequence == CTxIn::MAX_SEQUENCE_NONFINAL) continue;
        if (in.nSequence == MAX_BIP125_RBF_SEQUENCE) continue;
        assert(false);
    }
}
static util::Result<CreatedTransactionResult> CreateTransactionInternal(
        CWallet& wallet,
        const std::vector<CRecipient>& vecSend,
        int change_pos,
        const CCoinControl& coin_control,
        bool sign) EXCLUSIVE_LOCKS_REQUIRED(wallet.cs_wallet)
{
    AssertLockHeld(wallet.cs_wallet);
    CAmount nFeeRet;
    int nChangePosInOut = change_pos;
    FastRandomContext rng_fast;
    CMutableTransaction txNew;
    CoinSelectionParams coin_selection_params{rng_fast};
    coin_selection_params.m_avoid_partial_spends = coin_control.m_avoid_partial_spends;
    coin_selection_params.m_long_term_feerate = wallet.m_consolidate_feerate;
    CAmount recipients_sum = 0;
    const OutputType change_type = wallet.TransactionChangeType(coin_control.m_change_type ? *coin_control.m_change_type : wallet.m_default_change_type, vecSend);
    ReserveDestination reservedest(&wallet, change_type);
    unsigned int outputs_to_subtract_fee_from = 0;
    for (const auto& recipient : vecSend) {
        recipients_sum += recipient.nAmount;
        if (recipient.fSubtractFeeFromAmount) {
            outputs_to_subtract_fee_from++;
            coin_selection_params.m_subtract_fee_outputs = true;
        }
    }
    CScript scriptChange;
    bilingual_str error;
    if (!std::get_if<CNoDestination>(&coin_control.destChange)) {
        scriptChange = GetScriptForDestination(coin_control.destChange);
    } else {
        CTxDestination dest;
        auto op_dest = reservedest.GetReservedDestination(true);
        if (!op_dest) {
            error = _("Transaction needs a change address, but we can't generate it.") + Untranslated(" ") + util::ErrorString(op_dest);
        } else {
            dest = *op_dest;
            scriptChange = GetScriptForDestination(dest);
        }
        CHECK_NONFATAL(IsValidDestination(dest) != scriptChange.empty());
    }
    CTxOut change_prototype_txout(0, scriptChange);
    coin_selection_params.change_output_size = GetSerializeSize(change_prototype_txout);
    int change_spend_size = CalculateMaximumSignedInputSize(change_prototype_txout, &wallet);
    if (change_spend_size == -1) {
        coin_selection_params.change_spend_size = DUMMY_NESTED_P2WPKH_INPUT_SIZE;
    } else {
        coin_selection_params.change_spend_size = (size_t)change_spend_size;
    }
    coin_selection_params.m_discard_feerate = GetDiscardRate(wallet);
    FeeCalculation feeCalc;
    coin_selection_params.m_effective_feerate = GetMinimumFeeRate(wallet, coin_control, &feeCalc);
    if (coin_control.m_feerate && coin_selection_params.m_effective_feerate > *coin_control.m_feerate) {
        return util::Error{strprintf(_("Fee rate (%s) is lower than the minimum fee rate setting (%s)"), coin_control.m_feerate->ToString(FeeEstimateMode::ryp_VB), coin_selection_params.m_effective_feerate.ToString(FeeEstimateMode::ryp_VB))};
    }
    if (feeCalc.reason == FeeReason::FALLBACK && !wallet.m_allow_fallback_fee) {
        return util::Error{_("Fee estimation failed. Fallbackfee is disabled. Wait a few blocks or enable -fallbackfee.")};
    }
    coin_selection_params.m_change_fee = coin_selection_params.m_effective_feerate.GetFee(coin_selection_params.change_output_size);
    coin_selection_params.m_cost_of_change = coin_selection_params.m_discard_feerate.GetFee(coin_selection_params.change_spend_size) + coin_selection_params.m_change_fee;
    coin_selection_params.m_min_change_target = GenerateChangeTarget(std::floor(recipients_sum / vecSend.size()), coin_selection_params.m_change_fee, rng_fast);
    const auto dust = GetDustThreshold(change_prototype_txout, coin_selection_params.m_discard_feerate);
    const auto change_spend_fee = coin_selection_params.m_discard_feerate.GetFee(coin_selection_params.change_spend_size);
    coin_selection_params.min_viable_change = std::max(change_spend_fee + 1, dust);
    if (!coin_selection_params.m_subtract_fee_outputs) {
        coin_selection_params.tx_noinputs_size = 10;
        coin_selection_params.tx_noinputs_size += GetSizeOfCompactSize(vecSend.size());
    }
    for (const auto& recipient : vecSend)
    {
        CTxOut txout(recipient.nAmount, recipient.scriptPubKey);
        if (!coin_selection_params.m_subtract_fee_outputs) {
            coin_selection_params.tx_noinputs_size += ::GetSerializeSize(txout, PROTOCOL_VERSION);
        }
        if (IsDust(txout, wallet.chain().relayDustFee())) {
            return util::Error{_("Transaction amount too small")};
        }
        txNew.vout.push_back(txout);
    }
    const CAmount not_input_fees = coin_selection_params.m_effective_feerate.GetFee(coin_selection_params.tx_noinputs_size);
    CAmount selection_target = recipients_sum + not_input_fees;
    auto available_coins = AvailableCoins(wallet,
                                              &coin_control,
                                              coin_selection_params.m_effective_feerate,
                                              1,
                                              MAX_MONEY,
                                              MAX_MONEY,
                                              0);
    std::optional<SelectionResult> result = SelectCoins(wallet, available_coins,  selection_target, coin_control, coin_selection_params);
    if (!result) {
        return util::Error{_("Insufficient funds")};
    }
    TRACE5(coin_selection, selected_coins, wallet.GetName().c_str(), GetAlgorithmName(result->GetAlgo()).c_str(), result->GetTarget(), result->GetWaste(), result->GetSelectedValue());
    const CAmount change_amount = result->GetChange(coin_selection_params.min_viable_change, coin_selection_params.m_change_fee);
    if (change_amount > 0) {
        CTxOut newTxOut(change_amount, scriptChange);
        if (nChangePosInOut == -1) {
            nChangePosInOut = rng_fast.randrange(txNew.vout.size() + 1);
        } else if ((unsigned int)nChangePosInOut > txNew.vout.size()) {
            return util::Error{_("Transaction change output index out of range")};
        }
        txNew.vout.insert(txNew.vout.begin() + nChangePosInOut, newTxOut);
    } else {
        nChangePosInOut = -1;
    }
    std::vector<COutput> selected_coins = result->GetShuffledInputVector();
    const uint32_t nSequence{coin_control.m_signal_bip125_rbf.value_or(wallet.m_signal_rbf) ? MAX_BIP125_RBF_SEQUENCE : CTxIn::MAX_SEQUENCE_NONFINAL};
    for (const auto& coin : selected_coins) {
        txNew.vin.push_back(CTxIn(coin.outpoint, CScript(), nSequence));
    }
    DiscourageFeeSniping(txNew, rng_fast, wallet.chain(), wallet.GetLastBlockHash(), wallet.GetLastBlockHeight());
    TxSize tx_sizes = CalculateMaximumSignedTxSize(CTransaction(txNew), &wallet, &coin_control);
    int nBytes = tx_sizes.vsize;
    if (nBytes == -1) {
        return util::Error{_("Missing solving data for estimating transaction size")};
    }
    CAmount fee_needed = coin_selection_params.m_effective_feerate.GetFee(nBytes);
    nFeeRet = result->GetSelectedValue() - recipients_sum - change_amount;
    assert(coin_selection_params.m_subtract_fee_outputs || fee_needed <= nFeeRet);
    if (nChangePosInOut != -1 && fee_needed < nFeeRet) {
        auto& change = txNew.vout.at(nChangePosInOut);
        change.nValue += nFeeRet - fee_needed;
        nFeeRet = fee_needed;
    }
    if (coin_selection_params.m_subtract_fee_outputs) {
        CAmount to_reduce = fee_needed - nFeeRet;
        int i = 0;
        bool fFirst = true;
        for (const auto& recipient : vecSend)
        {
            if (i == nChangePosInOut) {
                ++i;
            }
            CTxOut& txout = txNew.vout[i];
            if (recipient.fSubtractFeeFromAmount)
            {
                txout.nValue -= to_reduce / outputs_to_subtract_fee_from;
                if (fFirst)
                {
                    fFirst = false;
                    txout.nValue -= to_reduce % outputs_to_subtract_fee_from;
                }
                if (IsDust(txout, wallet.chain().relayDustFee())) {
                    if (txout.nValue < 0) {
                        return util::Error{_("The transaction amount is too small to pay the fee")};
                    } else {
                        return util::Error{_("The transaction amount is too small to send after the fee has been deducted")};
                    }
                }
            }
            ++i;
        }
        nFeeRet = fee_needed;
    }
    if (scriptChange.empty() && nChangePosInOut != -1) {
        return util::Error{error};
    }
    if (sign && !wallet.SignTransaction(txNew)) {
        return util::Error{_("Signing transaction failed")};
    }
    CTransactionRef tx = MakeTransactionRef(std::move(txNew));
    if ((sign && GetTransactionWeight(*tx) > MAX_STANDARD_TX_WEIGHT) ||
        (!sign && tx_sizes.weight > MAX_STANDARD_TX_WEIGHT))
    {
        return util::Error{_("Transaction too large")};
    }
    if (nFeeRet > wallet.m_default_max_tx_fee) {
        return util::Error{TransactionErrorString(TransactionError::MAX_FEE_EXCEEDED)};
    }
    if (gArgs.GetBoolArg("-walletrejectlongchains", DEFAULT_WALLET_REJECT_LONG_CHAINS)) {
        if (!wallet.chain().checkChainLimits(tx)) {
            return util::Error{_("Transaction has too long of a mempool chain")};
        }
    }
    reservedest.KeepDestination();
    wallet.WalletLogPrintf("Fee Calculation: Fee:%d Bytes:%u Tgt:%d (requested %d) Reason:\"%s\" Decay %.5f: Estimation: (%g - %g) %.2f%% %.1f/(%.1f %d mem %.1f out) Fail: (%g - %g) %.2f%% %.1f/(%.1f %d mem %.1f out)\n",
              nFeeRet, nBytes, feeCalc.returnedTarget, feeCalc.desiredTarget, StringForFeeReason(feeCalc.reason), feeCalc.est.decay,
              feeCalc.est.pass.start, feeCalc.est.pass.end,
              (feeCalc.est.pass.totalConfirmed + feeCalc.est.pass.inMempool + feeCalc.est.pass.leftMempool) > 0.0 ? 100 * feeCalc.est.pass.withinTarget / (feeCalc.est.pass.totalConfirmed + feeCalc.est.pass.inMempool + feeCalc.est.pass.leftMempool) : 0.0,
              feeCalc.est.pass.withinTarget, feeCalc.est.pass.totalConfirmed, feeCalc.est.pass.inMempool, feeCalc.est.pass.leftMempool,
              feeCalc.est.fail.start, feeCalc.est.fail.end,
              (feeCalc.est.fail.totalConfirmed + feeCalc.est.fail.inMempool + feeCalc.est.fail.leftMempool) > 0.0 ? 100 * feeCalc.est.fail.withinTarget / (feeCalc.est.fail.totalConfirmed + feeCalc.est.fail.inMempool + feeCalc.est.fail.leftMempool) : 0.0,
              feeCalc.est.fail.withinTarget, feeCalc.est.fail.totalConfirmed, feeCalc.est.fail.inMempool, feeCalc.est.fail.leftMempool);
    return CreatedTransactionResult(tx, nFeeRet, nChangePosInOut, feeCalc);
}
util::Result<CreatedTransactionResult> CreateTransaction(
        CWallet& wallet,
        const std::vector<CRecipient>& vecSend,
        int change_pos,
        const CCoinControl& coin_control,
        bool sign)
{
    if (vecSend.empty()) {
        return util::Error{_("Transaction must have at least one recipient")};
    }
    if (std::any_of(vecSend.cbegin(), vecSend.cend(), [](const auto& recipient){ return recipient.nAmount < 0; })) {
        return util::Error{_("Transaction amounts must not be negative")};
    }
    LOCK(wallet.cs_wallet);
    auto res = CreateTransactionInternal(wallet, vecSend, change_pos, coin_control, sign);
    TRACE4(coin_selection, normal_create_tx_internal, wallet.GetName().c_str(), bool(res),
           res ? res->fee : 0, res ? res->change_pos : 0);
    if (!res) return res;
    const auto& txr_ungrouped = *res;
    if (txr_ungrouped.fee > 0   && wallet.m_max_aps_fee > -1 && !coin_control.m_avoid_partial_spends) {
        TRACE1(coin_selection, attempting_aps_create_tx, wallet.GetName().c_str());
        CCoinControl tmp_cc = coin_control;
        tmp_cc.m_avoid_partial_spends = true;
        auto txr_grouped = CreateTransactionInternal(wallet, vecSend, change_pos, tmp_cc, sign);
        const bool use_aps{txr_grouped.has_value() ? (txr_grouped->fee <= txr_ungrouped.fee + wallet.m_max_aps_fee) : false};
        TRACE5(coin_selection, aps_create_tx_internal, wallet.GetName().c_str(), use_aps, txr_grouped.has_value(),
               txr_grouped.has_value() ? txr_grouped->fee : 0, txr_grouped.has_value() ? txr_grouped->change_pos : 0);
        if (txr_grouped) {
            wallet.WalletLogPrintf("Fee non-grouped = %lld, grouped = %lld, using %s\n",
                txr_ungrouped.fee, txr_grouped->fee, use_aps ? "grouped" : "non-grouped");
            if (use_aps) return txr_grouped;
        }
    }
    return res;
}
bool FundTransaction(CWallet& wallet, CMutableTransaction& tx, CAmount& nFeeRet, int& nChangePosInOut, bilingual_str& error, bool lockUnspents, const std::set<int>& setSubtractFeeFromOutputs, CCoinControl coinControl)
{
    std::vector<CRecipient> vecSend;
    for (size_t idx = 0; idx < tx.vout.size(); idx++) {
        const CTxOut& txOut = tx.vout[idx];
        CRecipient recipient = {txOut.scriptPubKey, txOut.nValue, setSubtractFeeFromOutputs.count(idx) == 1};
        vecSend.push_back(recipient);
    }
    LOCK(wallet.cs_wallet);
    std::map<COutPoint, Coin> coins;
    for (const CTxIn& txin : tx.vin) {
        coins[txin.prevout];
    }
    wallet.chain().findCoins(coins);
    for (const CTxIn& txin : tx.vin) {
        const auto& outPoint = txin.prevout;
        if (wallet.IsMine(outPoint)) {
            coinControl.Select(outPoint);
        } else if (coins[outPoint].out.IsNull()) {
            error = _("Unable to find UTXO for external input");
            return false;
        } else {
            coinControl.SelectExternal(outPoint, coins[outPoint].out);
        }
    }
    auto res = CreateTransaction(wallet, vecSend, nChangePosInOut, coinControl, false);
    if (!res) {
        error = util::ErrorString(res);
        return false;
    }
    const auto& txr = *res;
    CTransactionRef tx_new = txr.tx;
    nFeeRet = txr.fee;
    nChangePosInOut = txr.change_pos;
    if (nChangePosInOut != -1) {
        tx.vout.insert(tx.vout.begin() + nChangePosInOut, tx_new->vout[nChangePosInOut]);
    }
    for (unsigned int idx = 0; idx < tx.vout.size(); idx++) {
        tx.vout[idx].nValue = tx_new->vout[idx].nValue;
    }
    for (const CTxIn& txin : tx_new->vin) {
        if (!coinControl.IsSelected(txin.prevout)) {
            tx.vin.push_back(txin);
        }
        if (lockUnspents) {
            wallet.LockCoin(txin.prevout);
        }
    }
    return true;
}
}
