#ifndef LCRYP_COMMON_BLOOM_H
#define LCRYP_COMMON_BLOOM_H
#include <serialize.h>
#include <span.h>
#include <vector>
class COutPoint;
class CTransaction;
static constexpr unsigned int MAX_BLOOM_FILTER_SIZE = 36000;
static constexpr unsigned int MAX_HASH_FUNCS = 50;
enum bloomflags
{
    BLOOM_UPDATE_NONE = 0,
    BLOOM_UPDATE_ALL = 1,
    BLOOM_UPDATE_P2PUBKEY_ONLY = 2,
    BLOOM_UPDATE_MASK = 3,
};
class CBloomFilter
{
private:
    std::vector<unsigned char> vData;
    unsigned int nHashFuncs;
    unsigned int nTweak;
    unsigned char nFlags;
    unsigned int Hash(unsigned int nHashNum, Span<const unsigned char> vDataToHash) const;
public:
    CBloomFilter(const unsigned int nElements, const double nFPRate, const unsigned int nTweak, unsigned char nFlagsIn);
    CBloomFilter() : nHashFuncs(0), nTweak(0), nFlags(0) {}
    SERIALIZE_METHODS(CBloomFilter, obj) { READWRITE(obj.vData, obj.nHashFuncs, obj.nTweak, obj.nFlags); }
    void insert(Span<const unsigned char> vKey);
    void insert(const COutPoint& outpoint);
    bool contains(Span<const unsigned char> vKey) const;
    bool contains(const COutPoint& outpoint) const;
    bool IsWithinSizeConstraints() const;
    bool IsRelevantAndUpdate(const CTransaction& tx);
};
class CRollingBloomFilter
{
public:
    CRollingBloomFilter(const unsigned int nElements, const double nFPRate);
    void insert(Span<const unsigned char> vKey);
    bool contains(Span<const unsigned char> vKey) const;
    void reset();
private:
    int nEntriesPerGeneration;
    int nEntriesThisGeneration;
    int nGeneration;
    std::vector<uint64_t> data;
    unsigned int nTweak;
    int nHashFuncs;
};
#endif
