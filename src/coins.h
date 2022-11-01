#ifndef LCRYP_COINS_H
#define LCRYP_COINS_H
#include <compressor.h>
#include <core_memusage.h>
#include <memusage.h>
#include <primitives/transaction.h>
#include <serialize.h>
#include <uint256.h>
#include <util/hasher.h>
#include <assert.h>
#include <stdint.h>
#include <functional>
#include <unordered_map>
class Coin
{
public:
    CTxOut out;
    unsigned int fCoinBase : 1;
    uint32_t nHeight : 31;
    Coin(CTxOut&& outIn, int nHeightIn, bool fCoinBaseIn) : out(std::move(outIn)), fCoinBase(fCoinBaseIn), nHeight(nHeightIn) {}
    Coin(const CTxOut& outIn, int nHeightIn, bool fCoinBaseIn) : out(outIn), fCoinBase(fCoinBaseIn),nHeight(nHeightIn) {}
    void Clear() {
        out.SetNull();
        fCoinBase = false;
        nHeight = 0;
    }
    Coin() : fCoinBase(false), nHeight(0) { }
    bool IsCoinBase() const {
        return fCoinBase;
    }
    template<typename Stream>
    void Serialize(Stream &s) const {
        assert(!IsSpent());
        uint32_t code = nHeight * uint32_t{2} + fCoinBase;
        ::Serialize(s, VARINT(code));
        ::Serialize(s, Using<TxOutCompression>(out));
    }
    template<typename Stream>
    void Unserialize(Stream &s) {
        uint32_t code = 0;
        ::Unserialize(s, VARINT(code));
        nHeight = code >> 1;
        fCoinBase = code & 1;
        ::Unserialize(s, Using<TxOutCompression>(out));
    }
    bool IsSpent() const {
        return out.IsNull();
    }
    size_t DynamicMemoryUsage() const {
        return memusage::DynamicUsage(out.scriptPubKey);
    }
};
struct CCoinsCacheEntry
{
    Coin coin;
    unsigned char flags;
    enum Flags {
        DIRTY = (1 << 0),
        FRESH = (1 << 1),
    };
    CCoinsCacheEntry() : flags(0) {}
    explicit CCoinsCacheEntry(Coin&& coin_) : coin(std::move(coin_)), flags(0) {}
    CCoinsCacheEntry(Coin&& coin_, unsigned char flag) : coin(std::move(coin_)), flags(flag) {}
};
typedef std::unordered_map<COutPoint, CCoinsCacheEntry, SaltedOutpointHasher> CCoinsMap;
class CCoinsViewCursor
{
public:
    CCoinsViewCursor(const uint256 &hashBlockIn): hashBlock(hashBlockIn) {}
    virtual ~CCoinsViewCursor() {}
    virtual bool GetKey(COutPoint &key) const = 0;
    virtual bool GetValue(Coin &coin) const = 0;
    virtual bool Valid() const = 0;
    virtual void Next() = 0;
    const uint256 &GetBestBlock() const { return hashBlock; }
private:
    uint256 hashBlock;
};
class CCoinsView
{
public:
    virtual bool GetCoin(const COutPoint &outpoint, Coin &coin) const;
    virtual bool HaveCoin(const COutPoint &outpoint) const;
    virtual uint256 GetBestBlock() const;
    virtual std::vector<uint256> GetHeadBlocks() const;
    virtual bool BatchWrite(CCoinsMap &mapCoins, const uint256 &hashBlock);
    virtual std::unique_ptr<CCoinsViewCursor> Cursor() const;
    virtual ~CCoinsView() {}
    virtual size_t EstimateSize() const { return 0; }
};
class CCoinsViewBacked : public CCoinsView
{
protected:
    CCoinsView *base;
public:
    CCoinsViewBacked(CCoinsView *viewIn);
    bool GetCoin(const COutPoint &outpoint, Coin &coin) const override;
    bool HaveCoin(const COutPoint &outpoint) const override;
    uint256 GetBestBlock() const override;
    std::vector<uint256> GetHeadBlocks() const override;
    void SetBackend(CCoinsView &viewIn);
    bool BatchWrite(CCoinsMap &mapCoins, const uint256 &hashBlock) override;
    std::unique_ptr<CCoinsViewCursor> Cursor() const override;
    size_t EstimateSize() const override;
};
class CCoinsViewCache : public CCoinsViewBacked
{
protected:
    mutable uint256 hashBlock;
    mutable CCoinsMap cacheCoins;
    mutable size_t cachedCoinsUsage;
public:
    CCoinsViewCache(CCoinsView *baseIn);
    CCoinsViewCache(const CCoinsViewCache &) = delete;
    bool GetCoin(const COutPoint &outpoint, Coin &coin) const override;
    bool HaveCoin(const COutPoint &outpoint) const override;
    uint256 GetBestBlock() const override;
    void SetBestBlock(const uint256 &hashBlock);
    bool BatchWrite(CCoinsMap &mapCoins, const uint256 &hashBlock) override;
    std::unique_ptr<CCoinsViewCursor> Cursor() const override {
        throw std::logic_error("CCoinsViewCache cursor iteration not supported.");
    }
    bool HaveCoinInCache(const COutPoint &outpoint) const;
    const Coin& AccessCoin(const COutPoint &output) const;
    void AddCoin(const COutPoint& outpoint, Coin&& coin, bool possible_overwrite);
    void EmplaceCoinInternalDANGER(COutPoint&& outpoint, Coin&& coin);
    bool SpendCoin(const COutPoint &outpoint, Coin* moveto = nullptr);
    bool Flush();
    void Uncache(const COutPoint &outpoint);
    unsigned int GetCacheSize() const;
    size_t DynamicMemoryUsage() const;
    bool HaveInputs(const CTransaction& tx) const;
    void ReallocateCache();
private:
    CCoinsMap::iterator FetchCoin(const COutPoint &outpoint) const;
};
void AddCoins(CCoinsViewCache& cache, const CTransaction& tx, int nHeight, bool check = false);
const Coin& AccessByTxid(const CCoinsViewCache& cache, const uint256& txid);
class CCoinsViewErrorCatcher final : public CCoinsViewBacked
{
public:
    explicit CCoinsViewErrorCatcher(CCoinsView* view) : CCoinsViewBacked(view) {}
    void AddReadErrCallback(std::function<void()> f) {
        m_err_callbacks.emplace_back(std::move(f));
    }
    bool GetCoin(const COutPoint &outpoint, Coin &coin) const override;
private:
    std::vector<std::function<void()>> m_err_callbacks;
};
#endif
