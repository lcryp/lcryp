#ifndef LCRYP_WALLET_TRANSACTION_H
#define LCRYP_WALLET_TRANSACTION_H
#include <consensus/amount.h>
#include <primitives/transaction.h>
#include <serialize.h>
#include <wallet/ismine.h>
#include <threadsafety.h>
#include <tinyformat.h>
#include <util/overloaded.h>
#include <util/strencodings.h>
#include <util/string.h>
#include <list>
#include <variant>
#include <vector>
namespace wallet {
struct TxStateConfirmed {
    uint256 confirmed_block_hash;
    int confirmed_block_height;
    int position_in_block;
    explicit TxStateConfirmed(const uint256& block_hash, int height, int index) : confirmed_block_hash(block_hash), confirmed_block_height(height), position_in_block(index) {}
};
struct TxStateInMempool {
};
struct TxStateConflicted {
    uint256 conflicting_block_hash;
    int conflicting_block_height;
    explicit TxStateConflicted(const uint256& block_hash, int height) : conflicting_block_hash(block_hash), conflicting_block_height(height) {}
};
struct TxStateInactive {
    bool abandoned;
    explicit TxStateInactive(bool abandoned = false) : abandoned(abandoned) {}
};
struct TxStateUnrecognized {
    uint256 block_hash;
    int index;
    TxStateUnrecognized(const uint256& block_hash, int index) : block_hash(block_hash), index(index) {}
};
using TxState = std::variant<TxStateConfirmed, TxStateInMempool, TxStateConflicted, TxStateInactive, TxStateUnrecognized>;
using SyncTxState = std::variant<TxStateConfirmed, TxStateInMempool, TxStateInactive>;
static inline TxState TxStateInterpretSerialized(TxStateUnrecognized data)
{
    if (data.block_hash == uint256::ZERO) {
        if (data.index == 0) return TxStateInactive{};
    } else if (data.block_hash == uint256::ONE) {
        if (data.index == -1) return TxStateInactive{ true};
    } else if (data.index >= 0) {
        return TxStateConfirmed{data.block_hash,  -1, data.index};
    } else if (data.index == -1) {
        return TxStateConflicted{data.block_hash,  -1};
    }
    return data;
}
static inline uint256 TxStateSerializedBlockHash(const TxState& state)
{
    return std::visit(util::Overloaded{
        [](const TxStateInactive& inactive) { return inactive.abandoned ? uint256::ONE : uint256::ZERO; },
        [](const TxStateInMempool& in_mempool) { return uint256::ZERO; },
        [](const TxStateConfirmed& confirmed) { return confirmed.confirmed_block_hash; },
        [](const TxStateConflicted& conflicted) { return conflicted.conflicting_block_hash; },
        [](const TxStateUnrecognized& unrecognized) { return unrecognized.block_hash; }
    }, state);
}
static inline int TxStateSerializedIndex(const TxState& state)
{
    return std::visit(util::Overloaded{
        [](const TxStateInactive& inactive) { return inactive.abandoned ? -1 : 0; },
        [](const TxStateInMempool& in_mempool) { return 0; },
        [](const TxStateConfirmed& confirmed) { return confirmed.position_in_block; },
        [](const TxStateConflicted& conflicted) { return -1; },
        [](const TxStateUnrecognized& unrecognized) { return unrecognized.index; }
    }, state);
}
typedef std::map<std::string, std::string> mapValue_t;
class CMerkleTx
{
public:
    template<typename Stream>
    void Unserialize(Stream& s)
    {
        CTransactionRef tx;
        uint256 hashBlock;
        std::vector<uint256> vMerkleBranch;
        int nIndex;
        s >> tx >> hashBlock >> vMerkleBranch >> nIndex;
    }
};
class CWalletTx
{
public:
    mapValue_t mapValue;
    std::vector<std::pair<std::string, std::string> > vOrderForm;
    unsigned int fTimeReceivedIsTxTime;
    unsigned int nTimeReceived;
    unsigned int nTimeSmart;
    bool fFromMe;
    int64_t nOrderPos;
    std::multimap<int64_t, CWalletTx*>::const_iterator m_it_wtxOrdered;
    enum AmountType { DEBIT, CREDIT, IMMATURE_CREDIT, AVAILABLE_CREDIT, AMOUNTTYPE_ENUM_ELEMENTS };
    mutable CachableAmount m_amounts[AMOUNTTYPE_ENUM_ELEMENTS];
    mutable bool m_is_cache_empty{true};
    mutable bool fChangeCached;
    mutable CAmount nChangeCached;
    CWalletTx(CTransactionRef tx, const TxState& state) : tx(std::move(tx)), m_state(state)
    {
        Init();
    }
    void Init()
    {
        mapValue.clear();
        vOrderForm.clear();
        fTimeReceivedIsTxTime = false;
        nTimeReceived = 0;
        nTimeSmart = 0;
        fFromMe = false;
        fChangeCached = false;
        nChangeCached = 0;
        nOrderPos = -1;
    }
    CTransactionRef tx;
    TxState m_state;
    template<typename Stream>
    void Serialize(Stream& s) const
    {
        mapValue_t mapValueCopy = mapValue;
        mapValueCopy["fromaccount"] = "";
        if (nOrderPos != -1) {
            mapValueCopy["n"] = ToString(nOrderPos);
        }
        if (nTimeSmart) {
            mapValueCopy["timesmart"] = strprintf("%u", nTimeSmart);
        }
        std::vector<uint8_t> dummy_vector1;
        std::vector<uint8_t> dummy_vector2;
        bool dummy_bool = false;
        uint256 serializedHash = TxStateSerializedBlockHash(m_state);
        int serializedIndex = TxStateSerializedIndex(m_state);
        s << tx << serializedHash << dummy_vector1 << serializedIndex << dummy_vector2 << mapValueCopy << vOrderForm << fTimeReceivedIsTxTime << nTimeReceived << fFromMe << dummy_bool;
    }
    template<typename Stream>
    void Unserialize(Stream& s)
    {
        Init();
        std::vector<uint256> dummy_vector1;
        std::vector<CMerkleTx> dummy_vector2;
        bool dummy_bool;
        uint256 serialized_block_hash;
        int serializedIndex;
        s >> tx >> serialized_block_hash >> dummy_vector1 >> serializedIndex >> dummy_vector2 >> mapValue >> vOrderForm >> fTimeReceivedIsTxTime >> nTimeReceived >> fFromMe >> dummy_bool;
        m_state = TxStateInterpretSerialized({serialized_block_hash, serializedIndex});
        const auto it_op = mapValue.find("n");
        nOrderPos = (it_op != mapValue.end()) ? LocaleIndependentAtoi<int64_t>(it_op->second) : -1;
        const auto it_ts = mapValue.find("timesmart");
        nTimeSmart = (it_ts != mapValue.end()) ? static_cast<unsigned int>(LocaleIndependentAtoi<int64_t>(it_ts->second)) : 0;
        mapValue.erase("fromaccount");
        mapValue.erase("spent");
        mapValue.erase("n");
        mapValue.erase("timesmart");
    }
    void SetTx(CTransactionRef arg)
    {
        tx = std::move(arg);
    }
    void MarkDirty()
    {
        m_amounts[DEBIT].Reset();
        m_amounts[CREDIT].Reset();
        m_amounts[IMMATURE_CREDIT].Reset();
        m_amounts[AVAILABLE_CREDIT].Reset();
        fChangeCached = false;
        m_is_cache_empty = true;
    }
    bool IsEquivalentTo(const CWalletTx& tx) const;
    bool InMempool() const;
    int64_t GetTxTime() const;
    template<typename T> const T* state() const { return std::get_if<T>(&m_state); }
    template<typename T> T* state() { return std::get_if<T>(&m_state); }
    bool isAbandoned() const { return state<TxStateInactive>() && state<TxStateInactive>()->abandoned; }
    bool isConflicted() const { return state<TxStateConflicted>(); }
    bool isUnconfirmed() const { return !isAbandoned() && !isConflicted() && !isConfirmed(); }
    bool isConfirmed() const { return state<TxStateConfirmed>(); }
    const uint256& GetHash() const { return tx->GetHash(); }
    const uint256& GetWitnessHash() const { return tx->GetWitnessHash(); }
    bool IsCoinBase() const { return tx->IsCoinBase(); }
    CWalletTx(CWalletTx const &) = delete;
    void operator=(CWalletTx const &x) = delete;
};
struct WalletTxOrderComparator {
    bool operator()(const CWalletTx* a, const CWalletTx* b) const
    {
        return a->nOrderPos < b->nOrderPos;
    }
};
}
#endif
