#include <consensus/amount.h>
#include <consensus/consensus.h>
#include <wallet/receive.h>
#include <wallet/transaction.h>
#include <wallet/wallet.h>
namespace wallet {
isminetype InputIsMine(const CWallet& wallet, const CTxIn& txin)
{
    AssertLockHeld(wallet.cs_wallet);
    const CWalletTx* prev = wallet.GetWalletTx(txin.prevout.hash);
    if (prev && txin.prevout.n < prev->tx->vout.size()) {
        return wallet.IsMine(prev->tx->vout[txin.prevout.n]);
    }
    return ISMINE_NO;
}
bool AllInputsMine(const CWallet& wallet, const CTransaction& tx, const isminefilter& filter)
{
    LOCK(wallet.cs_wallet);
    for (const CTxIn& txin : tx.vin) {
        if (!(InputIsMine(wallet, txin) & filter)) return false;
    }
    return true;
}
CAmount OutputGetCredit(const CWallet& wallet, const CTxOut& txout, const isminefilter& filter)
{
    if (!MoneyRange(txout.nValue))
        throw std::runtime_error(std::string(__func__) + ": value out of range");
    LOCK(wallet.cs_wallet);
    return ((wallet.IsMine(txout) & filter) ? txout.nValue : 0);
}
CAmount TxGetCredit(const CWallet& wallet, const CTransaction& tx, const isminefilter& filter)
{
    CAmount nCredit = 0;
    for (const CTxOut& txout : tx.vout)
    {
        nCredit += OutputGetCredit(wallet, txout, filter);
        if (!MoneyRange(nCredit))
            throw std::runtime_error(std::string(__func__) + ": value out of range");
    }
    return nCredit;
}
bool ScriptIsChange(const CWallet& wallet, const CScript& script)
{
    AssertLockHeld(wallet.cs_wallet);
    if (wallet.IsMine(script))
    {
        CTxDestination address;
        if (!ExtractDestination(script, address))
            return true;
        if (!wallet.FindAddressBookEntry(address)) {
            return true;
        }
    }
    return false;
}
bool OutputIsChange(const CWallet& wallet, const CTxOut& txout)
{
    return ScriptIsChange(wallet, txout.scriptPubKey);
}
CAmount OutputGetChange(const CWallet& wallet, const CTxOut& txout)
{
    AssertLockHeld(wallet.cs_wallet);
    if (!MoneyRange(txout.nValue))
        throw std::runtime_error(std::string(__func__) + ": value out of range");
    return (OutputIsChange(wallet, txout) ? txout.nValue : 0);
}
CAmount TxGetChange(const CWallet& wallet, const CTransaction& tx)
{
    LOCK(wallet.cs_wallet);
    CAmount nChange = 0;
    for (const CTxOut& txout : tx.vout)
    {
        nChange += OutputGetChange(wallet, txout);
        if (!MoneyRange(nChange))
            throw std::runtime_error(std::string(__func__) + ": value out of range");
    }
    return nChange;
}
static CAmount GetCachableAmount(const CWallet& wallet, const CWalletTx& wtx, CWalletTx::AmountType type, const isminefilter& filter)
{
    auto& amount = wtx.m_amounts[type];
    if (!amount.m_cached[filter]) {
        amount.Set(filter, type == CWalletTx::DEBIT ? wallet.GetDebit(*wtx.tx, filter) : TxGetCredit(wallet, *wtx.tx, filter));
        wtx.m_is_cache_empty = false;
    }
    return amount.m_value[filter];
}
CAmount CachedTxGetCredit(const CWallet& wallet, const CWalletTx& wtx, const isminefilter& filter)
{
    AssertLockHeld(wallet.cs_wallet);
    if (wallet.IsTxImmatureCoinBase(wtx))
        return 0;
    CAmount credit = 0;
    const isminefilter get_amount_filter{filter & ISMINE_ALL};
    if (get_amount_filter) {
        credit += GetCachableAmount(wallet, wtx, CWalletTx::CREDIT, get_amount_filter);
    }
    return credit;
}
CAmount CachedTxGetDebit(const CWallet& wallet, const CWalletTx& wtx, const isminefilter& filter)
{
    if (wtx.tx->vin.empty())
        return 0;
    CAmount debit = 0;
    const isminefilter get_amount_filter{filter & ISMINE_ALL};
    if (get_amount_filter) {
        debit += GetCachableAmount(wallet, wtx, CWalletTx::DEBIT, get_amount_filter);
    }
    return debit;
}
CAmount CachedTxGetChange(const CWallet& wallet, const CWalletTx& wtx)
{
    if (wtx.fChangeCached)
        return wtx.nChangeCached;
    wtx.nChangeCached = TxGetChange(wallet, *wtx.tx);
    wtx.fChangeCached = true;
    return wtx.nChangeCached;
}
CAmount CachedTxGetImmatureCredit(const CWallet& wallet, const CWalletTx& wtx, const isminefilter& filter)
{
    AssertLockHeld(wallet.cs_wallet);
    if (wallet.IsTxImmatureCoinBase(wtx) && wallet.IsTxInMainChain(wtx)) {
        return GetCachableAmount(wallet, wtx, CWalletTx::IMMATURE_CREDIT, filter);
    }
    return 0;
}
CAmount CachedTxGetAvailableCredit(const CWallet& wallet, const CWalletTx& wtx, const isminefilter& filter)
{
    AssertLockHeld(wallet.cs_wallet);
    bool allow_cache = (filter & ISMINE_ALL) && (filter & ISMINE_ALL) != ISMINE_ALL;
    if (wallet.IsTxImmatureCoinBase(wtx))
        return 0;
    if (allow_cache && wtx.m_amounts[CWalletTx::AVAILABLE_CREDIT].m_cached[filter]) {
        return wtx.m_amounts[CWalletTx::AVAILABLE_CREDIT].m_value[filter];
    }
    bool allow_used_addresses = (filter & ISMINE_USED) || !wallet.IsWalletFlagSet(WALLET_FLAG_AVOID_REUSE);
    CAmount nCredit = 0;
    uint256 hashTx = wtx.GetHash();
    for (unsigned int i = 0; i < wtx.tx->vout.size(); i++) {
        const CTxOut& txout = wtx.tx->vout[i];
        if (!wallet.IsSpent(COutPoint(hashTx, i)) && (allow_used_addresses || !wallet.IsSpentKey(txout.scriptPubKey))) {
            nCredit += OutputGetCredit(wallet, txout, filter);
            if (!MoneyRange(nCredit))
                throw std::runtime_error(std::string(__func__) + " : value out of range");
        }
    }
    if (allow_cache) {
        wtx.m_amounts[CWalletTx::AVAILABLE_CREDIT].Set(filter, nCredit);
        wtx.m_is_cache_empty = false;
    }
    return nCredit;
}
void CachedTxGetAmounts(const CWallet& wallet, const CWalletTx& wtx,
                  std::list<COutputEntry>& listReceived,
                  std::list<COutputEntry>& listSent, CAmount& nFee, const isminefilter& filter,
                  bool include_change)
{
    nFee = 0;
    listReceived.clear();
    listSent.clear();
    CAmount nDebit = CachedTxGetDebit(wallet, wtx, filter);
    if (nDebit > 0)
    {
        CAmount nValueOut = wtx.tx->GetValueOut();
        nFee = nDebit - nValueOut;
    }
    LOCK(wallet.cs_wallet);
    for (unsigned int i = 0; i < wtx.tx->vout.size(); ++i)
    {
        const CTxOut& txout = wtx.tx->vout[i];
        isminetype fIsMine = wallet.IsMine(txout);
        if (nDebit > 0)
        {
            if (!include_change && OutputIsChange(wallet, txout))
                continue;
        }
        else if (!(fIsMine & filter))
            continue;
        CTxDestination address;
        if (!ExtractDestination(txout.scriptPubKey, address) && !txout.scriptPubKey.IsUnspendable())
        {
            wallet.WalletLogPrintf("CWalletTx::GetAmounts: Unknown transaction type found, txid %s\n",
                                    wtx.GetHash().ToString());
            address = CNoDestination();
        }
        COutputEntry output = {address, txout.nValue, (int)i};
        if (nDebit > 0)
            listSent.push_back(output);
        if (fIsMine & filter)
            listReceived.push_back(output);
    }
}
bool CachedTxIsFromMe(const CWallet& wallet, const CWalletTx& wtx, const isminefilter& filter)
{
    return (CachedTxGetDebit(wallet, wtx, filter) > 0);
}
bool CachedTxIsTrusted(const CWallet& wallet, const CWalletTx& wtx, std::set<uint256>& trusted_parents)
{
    AssertLockHeld(wallet.cs_wallet);
    int nDepth = wallet.GetTxDepthInMainChain(wtx);
    if (nDepth >= 1) return true;
    if (nDepth < 0) return false;
    if (!wallet.m_spend_zero_conf_change || !CachedTxIsFromMe(wallet, wtx, ISMINE_ALL)) return false;
    if (!wtx.InMempool()) return false;
    for (const CTxIn& txin : wtx.tx->vin)
    {
        const CWalletTx* parent = wallet.GetWalletTx(txin.prevout.hash);
        if (parent == nullptr) return false;
        const CTxOut& parentOut = parent->tx->vout[txin.prevout.n];
        if (wallet.IsMine(parentOut) != ISMINE_SPENDABLE) return false;
        if (trusted_parents.count(parent->GetHash())) continue;
        if (!CachedTxIsTrusted(wallet, *parent, trusted_parents)) return false;
        trusted_parents.insert(parent->GetHash());
    }
    return true;
}
bool CachedTxIsTrusted(const CWallet& wallet, const CWalletTx& wtx)
{
    std::set<uint256> trusted_parents;
    LOCK(wallet.cs_wallet);
    return CachedTxIsTrusted(wallet, wtx, trusted_parents);
}
Balance GetBalance(const CWallet& wallet, const int min_depth, bool avoid_reuse)
{
    Balance ret;
    isminefilter reuse_filter = avoid_reuse ? ISMINE_NO : ISMINE_USED;
    {
        LOCK(wallet.cs_wallet);
        std::set<uint256> trusted_parents;
        for (const auto& entry : wallet.mapWallet)
        {
            const CWalletTx& wtx = entry.second;
            const bool is_trusted{CachedTxIsTrusted(wallet, wtx, trusted_parents)};
            const int tx_depth{wallet.GetTxDepthInMainChain(wtx)};
            const CAmount tx_credit_mine{CachedTxGetAvailableCredit(wallet, wtx, ISMINE_SPENDABLE | reuse_filter)};
            const CAmount tx_credit_watchonly{CachedTxGetAvailableCredit(wallet, wtx, ISMINE_WATCH_ONLY | reuse_filter)};
            if (is_trusted && tx_depth >= min_depth) {
                ret.m_mine_trusted += tx_credit_mine;
                ret.m_watchonly_trusted += tx_credit_watchonly;
            }
            if (!is_trusted && tx_depth == 0 && wtx.InMempool()) {
                ret.m_mine_untrusted_pending += tx_credit_mine;
                ret.m_watchonly_untrusted_pending += tx_credit_watchonly;
            }
            ret.m_mine_immature += CachedTxGetImmatureCredit(wallet, wtx, ISMINE_SPENDABLE);
            ret.m_watchonly_immature += CachedTxGetImmatureCredit(wallet, wtx, ISMINE_WATCH_ONLY);
        }
    }
    return ret;
}
std::map<CTxDestination, CAmount> GetAddressBalances(const CWallet& wallet)
{
    std::map<CTxDestination, CAmount> balances;
    {
        LOCK(wallet.cs_wallet);
        std::set<uint256> trusted_parents;
        for (const auto& walletEntry : wallet.mapWallet)
        {
            const CWalletTx& wtx = walletEntry.second;
            if (!CachedTxIsTrusted(wallet, wtx, trusted_parents))
                continue;
            if (wallet.IsTxImmatureCoinBase(wtx))
                continue;
            int nDepth = wallet.GetTxDepthInMainChain(wtx);
            if (nDepth < (CachedTxIsFromMe(wallet, wtx, ISMINE_ALL) ? 0 : 1))
                continue;
            for (unsigned int i = 0; i < wtx.tx->vout.size(); i++) {
                const auto& output = wtx.tx->vout[i];
                CTxDestination addr;
                if (!wallet.IsMine(output))
                    continue;
                if(!ExtractDestination(output.scriptPubKey, addr))
                    continue;
                CAmount n = wallet.IsSpent(COutPoint(walletEntry.first, i)) ? 0 : output.nValue;
                balances[addr] += n;
            }
        }
    }
    return balances;
}
std::set< std::set<CTxDestination> > GetAddressGroupings(const CWallet& wallet)
{
    AssertLockHeld(wallet.cs_wallet);
    std::set< std::set<CTxDestination> > groupings;
    std::set<CTxDestination> grouping;
    for (const auto& walletEntry : wallet.mapWallet)
    {
        const CWalletTx& wtx = walletEntry.second;
        if (wtx.tx->vin.size() > 0)
        {
            bool any_mine = false;
            for (const CTxIn& txin : wtx.tx->vin)
            {
                CTxDestination address;
                if(!InputIsMine(wallet, txin))
                    continue;
                if(!ExtractDestination(wallet.mapWallet.at(txin.prevout.hash).tx->vout[txin.prevout.n].scriptPubKey, address))
                    continue;
                grouping.insert(address);
                any_mine = true;
            }
            if (any_mine)
            {
               for (const CTxOut& txout : wtx.tx->vout)
                   if (OutputIsChange(wallet, txout))
                   {
                       CTxDestination txoutAddr;
                       if(!ExtractDestination(txout.scriptPubKey, txoutAddr))
                           continue;
                       grouping.insert(txoutAddr);
                   }
            }
            if (grouping.size() > 0)
            {
                groupings.insert(grouping);
                grouping.clear();
            }
        }
        for (const auto& txout : wtx.tx->vout)
            if (wallet.IsMine(txout))
            {
                CTxDestination address;
                if(!ExtractDestination(txout.scriptPubKey, address))
                    continue;
                grouping.insert(address);
                groupings.insert(grouping);
                grouping.clear();
            }
    }
    std::set< std::set<CTxDestination>* > uniqueGroupings;
    std::map< CTxDestination, std::set<CTxDestination>* > setmap;
    for (const std::set<CTxDestination>& _grouping : groupings)
    {
        std::set< std::set<CTxDestination>* > hits;
        std::map< CTxDestination, std::set<CTxDestination>* >::iterator it;
        for (const CTxDestination& address : _grouping)
            if ((it = setmap.find(address)) != setmap.end())
                hits.insert((*it).second);
        std::set<CTxDestination>* merged = new std::set<CTxDestination>(_grouping);
        for (std::set<CTxDestination>* hit : hits)
        {
            merged->insert(hit->begin(), hit->end());
            uniqueGroupings.erase(hit);
            delete hit;
        }
        uniqueGroupings.insert(merged);
        for (const CTxDestination& element : *merged)
            setmap[element] = merged;
    }
    std::set< std::set<CTxDestination> > ret;
    for (const std::set<CTxDestination>* uniqueGrouping : uniqueGroupings)
    {
        ret.insert(*uniqueGrouping);
        delete uniqueGrouping;
    }
    return ret;
}
}
