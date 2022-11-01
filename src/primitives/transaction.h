#ifndef LCRYP_PRIMITIVES_TRANSACTION_H
#define LCRYP_PRIMITIVES_TRANSACTION_H
#include <consensus/amount.h>
#include <prevector.h>
#include <script/script.h>
#include <serialize.h>
#include <uint256.h>
#include <cstddef>
#include <cstdint>
#include <ios>
#include <limits>
#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include <vector>
static const int SERIALIZE_TRANSACTION_NO_WITNESS = 0x40000000;
class COutPoint
{
public:
    uint256 hash;
    uint32_t n;
    static constexpr uint32_t NULL_INDEX = std::numeric_limits<uint32_t>::max();
    COutPoint(): n(NULL_INDEX) { }
    COutPoint(const uint256& hashIn, uint32_t nIn): hash(hashIn), n(nIn) { }
    SERIALIZE_METHODS(COutPoint, obj) { READWRITE(obj.hash, obj.n); }
    void SetNull() { hash.SetNull(); n = NULL_INDEX; }
    bool IsNull() const { return (hash.IsNull() && n == NULL_INDEX); }
    friend bool operator<(const COutPoint& a, const COutPoint& b)
    {
        int cmp = a.hash.Compare(b.hash);
        return cmp < 0 || (cmp == 0 && a.n < b.n);
    }
    friend bool operator==(const COutPoint& a, const COutPoint& b)
    {
        return (a.hash == b.hash && a.n == b.n);
    }
    friend bool operator!=(const COutPoint& a, const COutPoint& b)
    {
        return !(a == b);
    }
    std::string ToString() const;
};
class CTxIn
{
public:
    COutPoint prevout;
    CScript scriptSig;
    uint32_t nSequence;
    CScriptWitness scriptWitness;
    static const uint32_t SEQUENCE_FINAL = 0xffffffff;
    static const uint32_t MAX_SEQUENCE_NONFINAL{SEQUENCE_FINAL - 1};
    static const uint32_t SEQUENCE_LOCKTIME_DISABLE_FLAG = (1U << 31);
    static const uint32_t SEQUENCE_LOCKTIME_TYPE_FLAG = (1 << 22);
    static const uint32_t SEQUENCE_LOCKTIME_MASK = 0x0000ffff;
    static const int SEQUENCE_LOCKTIME_GRANULARITY = 9;
    CTxIn()
    {
        nSequence = SEQUENCE_FINAL;
    }
    explicit CTxIn(COutPoint prevoutIn, CScript scriptSigIn=CScript(), uint32_t nSequenceIn=SEQUENCE_FINAL);
    CTxIn(uint256 hashPrevTx, uint32_t nOut, CScript scriptSigIn=CScript(), uint32_t nSequenceIn=SEQUENCE_FINAL);
    SERIALIZE_METHODS(CTxIn, obj) { READWRITE(obj.prevout, obj.scriptSig, obj.nSequence); }
    friend bool operator==(const CTxIn& a, const CTxIn& b)
    {
        return (a.prevout   == b.prevout &&
                a.scriptSig == b.scriptSig &&
                a.nSequence == b.nSequence);
    }
    friend bool operator!=(const CTxIn& a, const CTxIn& b)
    {
        return !(a == b);
    }
    std::string ToString() const;
};
class CTxOut
{
public:
    CAmount nValue;
    CScript scriptPubKey;
    CTxOut()
    {
        SetNull();
    }
    CTxOut(const CAmount& nValueIn, CScript scriptPubKeyIn);
    SERIALIZE_METHODS(CTxOut, obj) { READWRITE(obj.nValue, obj.scriptPubKey); }
    void SetNull()
    {
        nValue = -1;
        scriptPubKey.clear();
    }
    bool IsNull() const
    {
        return (nValue == -1);
    }
    friend bool operator==(const CTxOut& a, const CTxOut& b)
    {
        return (a.nValue       == b.nValue &&
                a.scriptPubKey == b.scriptPubKey);
    }
    friend bool operator!=(const CTxOut& a, const CTxOut& b)
    {
        return !(a == b);
    }
    std::string ToString() const;
};
struct CMutableTransaction;
template<typename Stream, typename TxType>
inline void UnserializeTransaction(TxType& tx, Stream& s) {
    const bool fAllowWitness = !(s.GetVersion() & SERIALIZE_TRANSACTION_NO_WITNESS);
    s >> tx.nVersion;
    unsigned char flags = 0;
    tx.vin.clear();
    tx.vout.clear();
    s >> tx.vin;
    if (tx.vin.size() == 0 && fAllowWitness) {
        s >> flags;
        if (flags != 0) {
            s >> tx.vin;
            s >> tx.vout;
        }
    } else {
        s >> tx.vout;
    }
    if ((flags & 1) && fAllowWitness) {
        flags ^= 1;
        for (size_t i = 0; i < tx.vin.size(); i++) {
            s >> tx.vin[i].scriptWitness.stack;
        }
        if (!tx.HasWitness()) {
            throw std::ios_base::failure("Superfluous witness record");
        }
    }
    if (flags) {
        throw std::ios_base::failure("Unknown transaction optional data");
    }
    s >> tx.nLockTime;
}
template<typename Stream, typename TxType>
inline void SerializeTransaction(const TxType& tx, Stream& s) {
    const bool fAllowWitness = !(s.GetVersion() & SERIALIZE_TRANSACTION_NO_WITNESS);
    s << tx.nVersion;
    unsigned char flags = 0;
    if (fAllowWitness) {
        if (tx.HasWitness()) {
            flags |= 1;
        }
    }
    if (flags) {
        std::vector<CTxIn> vinDummy;
        s << vinDummy;
        s << flags;
    }
    s << tx.vin;
    s << tx.vout;
    if (flags & 1) {
        for (size_t i = 0; i < tx.vin.size(); i++) {
            s << tx.vin[i].scriptWitness.stack;
        }
    }
    s << tx.nLockTime;
}
class CTransaction
{
public:
    static const int32_t CURRENT_VERSION=2;
    const std::vector<CTxIn> vin;
    const std::vector<CTxOut> vout;
    const int32_t nVersion;
    const uint32_t nLockTime;
private:
    const uint256 hash;
    const uint256 m_witness_hash;
    uint256 ComputeHash() const;
    uint256 ComputeWitnessHash() const;
public:
    explicit CTransaction(const CMutableTransaction& tx);
    explicit CTransaction(CMutableTransaction&& tx);
    template <typename Stream>
    inline void Serialize(Stream& s) const {
        SerializeTransaction(*this, s);
    }
    template <typename Stream>
    CTransaction(deserialize_type, Stream& s) : CTransaction(CMutableTransaction(deserialize, s)) {}
    bool IsNull() const {
        return vin.empty() && vout.empty();
    }
    const uint256& GetHash() const { return hash; }
    const uint256& GetWitnessHash() const { return m_witness_hash; };
    CAmount GetValueOut() const;
    unsigned int GetTotalSize() const;
    bool IsCoinBase() const
    {
        return (vin.size() == 1 && vin[0].prevout.IsNull());
    }
    friend bool operator==(const CTransaction& a, const CTransaction& b)
    {
        return a.hash == b.hash;
    }
    friend bool operator!=(const CTransaction& a, const CTransaction& b)
    {
        return a.hash != b.hash;
    }
    std::string ToString() const;
    bool HasWitness() const
    {
        for (size_t i = 0; i < vin.size(); i++) {
            if (!vin[i].scriptWitness.IsNull()) {
                return true;
            }
        }
        return false;
    }
};
struct CMutableTransaction
{
    std::vector<CTxIn> vin;
    std::vector<CTxOut> vout;
    int32_t nVersion;
    uint32_t nLockTime;
    explicit CMutableTransaction();
    explicit CMutableTransaction(const CTransaction& tx);
    template <typename Stream>
    inline void Serialize(Stream& s) const {
        SerializeTransaction(*this, s);
    }
    template <typename Stream>
    inline void Unserialize(Stream& s) {
        UnserializeTransaction(*this, s);
    }
    template <typename Stream>
    CMutableTransaction(deserialize_type, Stream& s) {
        Unserialize(s);
    }
    uint256 GetHash() const;
    bool HasWitness() const
    {
        for (size_t i = 0; i < vin.size(); i++) {
            if (!vin[i].scriptWitness.IsNull()) {
                return true;
            }
        }
        return false;
    }
};
typedef std::shared_ptr<const CTransaction> CTransactionRef;
template <typename Tx> static inline CTransactionRef MakeTransactionRef(Tx&& txIn) { return std::make_shared<const CTransaction>(std::forward<Tx>(txIn)); }
class GenTxid
{
    bool m_is_wtxid;
    uint256 m_hash;
    GenTxid(bool is_wtxid, const uint256& hash) : m_is_wtxid(is_wtxid), m_hash(hash) {}
public:
    static GenTxid Txid(const uint256& hash) { return GenTxid{false, hash}; }
    static GenTxid Wtxid(const uint256& hash) { return GenTxid{true, hash}; }
    bool IsWtxid() const { return m_is_wtxid; }
    const uint256& GetHash() const { return m_hash; }
    friend bool operator==(const GenTxid& a, const GenTxid& b) { return a.m_is_wtxid == b.m_is_wtxid && a.m_hash == b.m_hash; }
    friend bool operator<(const GenTxid& a, const GenTxid& b) { return std::tie(a.m_is_wtxid, a.m_hash) < std::tie(b.m_is_wtxid, b.m_hash); }
};
#endif
