#include <consensus/validation.h>
#include <interfaces/chain.h>
#include <policy/fees.h>
#include <policy/policy.h>
#include <util/moneystr.h>
#include <util/rbf.h>
#include <util/system.h>
#include <util/translation.h>
#include <wallet/coincontrol.h>
#include <wallet/feebumper.h>
#include <wallet/fees.h>
#include <wallet/receive.h>
#include <wallet/spend.h>
#include <wallet/wallet.h>
namespace wallet {
static feebumper::Result PreconditionChecks(const CWallet& wallet, const CWalletTx& wtx, bool require_mine, std::vector<bilingual_str>& errors) EXCLUSIVE_LOCKS_REQUIRED(wallet.cs_wallet)
{
    if (wallet.HasWalletSpend(wtx.tx)) {
        errors.push_back(Untranslated("Transaction has descendants in the wallet"));
        return feebumper::Result::INVALID_PARAMETER;
    }
    {
        if (wallet.chain().hasDescendantsInMempool(wtx.GetHash())) {
            errors.push_back(Untranslated("Transaction has descendants in the mempool"));
            return feebumper::Result::INVALID_PARAMETER;
        }
    }
    if (wallet.GetTxDepthInMainChain(wtx) != 0) {
        errors.push_back(Untranslated("Transaction has been mined, or is conflicted with a mined transaction"));
        return feebumper::Result::WALLET_ERROR;
    }
    if (!SignalsOptInRBF(*wtx.tx)) {
        errors.push_back(Untranslated("Transaction is not BIP 125 replaceable"));
        return feebumper::Result::WALLET_ERROR;
    }
    if (wtx.mapValue.count("replaced_by_txid")) {
        errors.push_back(strprintf(Untranslated("Cannot bump transaction %s which was already bumped by transaction %s"), wtx.GetHash().ToString(), wtx.mapValue.at("replaced_by_txid")));
        return feebumper::Result::WALLET_ERROR;
    }
    if (require_mine) {
        isminefilter filter = wallet.GetLegacyScriptPubKeyMan() && wallet.IsWalletFlagSet(WALLET_FLAG_DISABLE_PRIVATE_KEYS) ? ISMINE_WATCH_ONLY : ISMINE_SPENDABLE;
        if (!AllInputsMine(wallet, *wtx.tx, filter)) {
            errors.push_back(Untranslated("Transaction contains inputs that don't belong to this wallet"));
            return feebumper::Result::WALLET_ERROR;
        }
    }
    return feebumper::Result::OK;
}
static feebumper::Result CheckFeeRate(const CWallet& wallet, const CWalletTx& wtx, const CFeeRate& newFeerate, const int64_t maxTxSize, CAmount old_fee, std::vector<bilingual_str>& errors)
{
    CFeeRate minMempoolFeeRate = wallet.chain().mempoolMinFee();
    if (newFeerate.GetFeePerK() < minMempoolFeeRate.GetFeePerK()) {
        errors.push_back(strprintf(
            Untranslated("New fee rate (%s) is lower than the minimum fee rate (%s) to get into the mempool -- "),
            FormatMoney(newFeerate.GetFeePerK()),
            FormatMoney(minMempoolFeeRate.GetFeePerK())));
        return feebumper::Result::WALLET_ERROR;
    }
    CAmount new_total_fee = newFeerate.GetFee(maxTxSize);
    CFeeRate incrementalRelayFee = std::max(wallet.chain().relayIncrementalFee(), CFeeRate(WALLET_INCREMENTAL_RELAY_FEE));
    const int64_t txSize = GetVirtualTransactionSize(*(wtx.tx));
    CFeeRate nOldFeeRate(old_fee, txSize);
    CAmount minTotalFee = nOldFeeRate.GetFee(maxTxSize) + incrementalRelayFee.GetFee(maxTxSize);
    if (new_total_fee < minTotalFee) {
        errors.push_back(strprintf(Untranslated("Insufficient total fee %s, must be at least %s (oldFee %s + incrementalFee %s)"),
            FormatMoney(new_total_fee), FormatMoney(minTotalFee), FormatMoney(nOldFeeRate.GetFee(maxTxSize)), FormatMoney(incrementalRelayFee.GetFee(maxTxSize))));
        return feebumper::Result::INVALID_PARAMETER;
    }
    CAmount requiredFee = GetRequiredFee(wallet, maxTxSize);
    if (new_total_fee < requiredFee) {
        errors.push_back(strprintf(Untranslated("Insufficient total fee (cannot be less than required fee %s)"),
            FormatMoney(requiredFee)));
        return feebumper::Result::INVALID_PARAMETER;
    }
    const CAmount max_tx_fee = wallet.m_default_max_tx_fee;
    if (new_total_fee > max_tx_fee) {
        errors.push_back(strprintf(Untranslated("Specified or calculated fee %s is too high (cannot be higher than -maxtxfee %s)"),
            FormatMoney(new_total_fee), FormatMoney(max_tx_fee)));
        return feebumper::Result::WALLET_ERROR;
    }
    return feebumper::Result::OK;
}
static CFeeRate EstimateFeeRate(const CWallet& wallet, const CWalletTx& wtx, const CAmount old_fee, const CCoinControl& coin_control)
{
    int64_t txSize = GetVirtualTransactionSize(*(wtx.tx));
    CFeeRate feerate(old_fee, txSize);
    feerate += CFeeRate(1);
    CFeeRate node_incremental_relay_fee = wallet.chain().relayIncrementalFee();
    CFeeRate wallet_incremental_relay_fee = CFeeRate(WALLET_INCREMENTAL_RELAY_FEE);
    feerate += std::max(node_incremental_relay_fee, wallet_incremental_relay_fee);
    CFeeRate min_feerate(GetMinimumFeeRate(wallet, coin_control,  nullptr));
    return std::max(feerate, min_feerate);
}
namespace feebumper {
bool TransactionCanBeBumped(const CWallet& wallet, const uint256& txid)
{
    LOCK(wallet.cs_wallet);
    const CWalletTx* wtx = wallet.GetWalletTx(txid);
    if (wtx == nullptr) return false;
    std::vector<bilingual_str> errors_dummy;
    feebumper::Result res = PreconditionChecks(wallet, *wtx,   true, errors_dummy);
    return res == feebumper::Result::OK;
}
Result CreateRateBumpTransaction(CWallet& wallet, const uint256& txid, const CCoinControl& coin_control, std::vector<bilingual_str>& errors,
                                 CAmount& old_fee, CAmount& new_fee, CMutableTransaction& mtx, bool require_mine)
{
    CCoinControl new_coin_control(coin_control);
    LOCK(wallet.cs_wallet);
    errors.clear();
    auto it = wallet.mapWallet.find(txid);
    if (it == wallet.mapWallet.end()) {
        errors.push_back(Untranslated("Invalid or non-wallet transaction id"));
        return Result::INVALID_ADDRESS_OR_KEY;
    }
    const CWalletTx& wtx = it->second;
    std::map<COutPoint, Coin> coins;
    CAmount input_value = 0;
    std::vector<CTxOut> spent_outputs;
    for (const CTxIn& txin : wtx.tx->vin) {
        coins[txin.prevout];
    }
    wallet.chain().findCoins(coins);
    for (const CTxIn& txin : wtx.tx->vin) {
        const Coin& coin = coins.at(txin.prevout);
        if (coin.out.IsNull()) {
            errors.push_back(Untranslated(strprintf("%s:%u is already spent", txin.prevout.hash.GetHex(), txin.prevout.n)));
            return Result::MISC_ERROR;
        }
        if (wallet.IsMine(txin.prevout)) {
            new_coin_control.Select(txin.prevout);
        } else {
            new_coin_control.SelectExternal(txin.prevout, coin.out);
        }
        input_value += coin.out.nValue;
        spent_outputs.push_back(coin.out);
    }
    PrecomputedTransactionData txdata;
    txdata.Init(*wtx.tx, std::move(spent_outputs),   true);
    for (unsigned int i = 0; i < wtx.tx->vin.size(); ++i) {
        const CTxIn& txin = wtx.tx->vin.at(i);
        const Coin& coin = coins.at(txin.prevout);
        if (new_coin_control.IsExternalSelected(txin.prevout)) {
            int64_t input_weight = GetTransactionInputWeight(txin);
            SignatureWeights weights;
            TransactionSignatureChecker tx_checker(wtx.tx.get(), i, coin.out.nValue, txdata, MissingDataBehavior::FAIL);
            SignatureWeightChecker size_checker(weights, tx_checker);
            VerifyScript(txin.scriptSig, coin.out.scriptPubKey, &txin.scriptWitness, STANDARD_SCRIPT_VERIFY_FLAGS, size_checker);
            input_weight += weights.GetWeightDiffToMax();
            new_coin_control.SetInputWeight(txin.prevout, input_weight);
        }
    }
    Result result = PreconditionChecks(wallet, wtx, require_mine, errors);
    if (result != Result::OK) {
        return result;
    }
    std::vector<CRecipient> recipients;
    CAmount output_value = 0;
    for (const auto& output : wtx.tx->vout) {
        if (!OutputIsChange(wallet, output)) {
            CRecipient recipient = {output.scriptPubKey, output.nValue, false};
            recipients.push_back(recipient);
        } else {
            CTxDestination change_dest;
            ExtractDestination(output.scriptPubKey, change_dest);
            new_coin_control.destChange = change_dest;
        }
        output_value += output.nValue;
    }
    old_fee = input_value - output_value;
    if (coin_control.m_feerate) {
        CMutableTransaction mtx{*wtx.tx};
        for (auto& txin : mtx.vin) {
            txin.scriptSig.clear();
            txin.scriptWitness.SetNull();
        }
        const int64_t maxTxSize{CalculateMaximumSignedTxSize(CTransaction(mtx), &wallet, &new_coin_control).vsize};
        Result res = CheckFeeRate(wallet, wtx, *new_coin_control.m_feerate, maxTxSize, old_fee, errors);
        if (res != Result::OK) {
            return res;
        }
    } else {
        new_coin_control.m_feerate = EstimateFeeRate(wallet, wtx, old_fee, new_coin_control);
    }
    for (const auto& inputs : wtx.tx->vin) {
        new_coin_control.Select(COutPoint(inputs.prevout));
    }
    new_coin_control.m_allow_other_inputs = true;
    new_coin_control.m_min_depth = 1;
    constexpr int RANDOM_CHANGE_POSITION = -1;
    auto res = CreateTransaction(wallet, recipients, RANDOM_CHANGE_POSITION, new_coin_control, false);
    if (!res) {
        errors.push_back(Untranslated("Unable to create transaction.") + Untranslated(" ") + util::ErrorString(res));
        return Result::WALLET_ERROR;
    }
    const auto& txr = *res;
    new_fee = txr.fee;
    mtx = CMutableTransaction(*txr.tx);
    return Result::OK;
}
bool SignTransaction(CWallet& wallet, CMutableTransaction& mtx) {
    LOCK(wallet.cs_wallet);
    return wallet.SignTransaction(mtx);
}
Result CommitTransaction(CWallet& wallet, const uint256& txid, CMutableTransaction&& mtx, std::vector<bilingual_str>& errors, uint256& bumped_txid)
{
    LOCK(wallet.cs_wallet);
    if (!errors.empty()) {
        return Result::MISC_ERROR;
    }
    auto it = txid.IsNull() ? wallet.mapWallet.end() : wallet.mapWallet.find(txid);
    if (it == wallet.mapWallet.end()) {
        errors.push_back(Untranslated("Invalid or non-wallet transaction id"));
        return Result::MISC_ERROR;
    }
    const CWalletTx& oldWtx = it->second;
    Result result = PreconditionChecks(wallet, oldWtx,   false, errors);
    if (result != Result::OK) {
        return result;
    }
    CTransactionRef tx = MakeTransactionRef(std::move(mtx));
    mapValue_t mapValue = oldWtx.mapValue;
    mapValue["replaces_txid"] = oldWtx.GetHash().ToString();
    wallet.CommitTransaction(tx, std::move(mapValue), oldWtx.vOrderForm);
    bumped_txid = tx->GetHash();
    if (!wallet.MarkReplaced(oldWtx.GetHash(), bumped_txid)) {
        errors.push_back(Untranslated("Created new bumpfee transaction but could not mark the original transaction as replaced"));
    }
    return Result::OK;
}
}
}
