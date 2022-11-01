#ifndef LCRYP_UTIL_HASHER_H
#define LCRYP_UTIL_HASHER_H
#include <crypto/common.h>
#include <crypto/siphash.h>
#include <primitives/transaction.h>
#include <uint256.h>
#include <cstdint>
#include <cstring>
template <typename C> class Span;
class SaltedTxidHasher
{
private:
    const uint64_t k0, k1;
public:
    SaltedTxidHasher();
    size_t operator()(const uint256& txid) const {
        return SipHashUint256(k0, k1, txid);
    }
};
class SaltedOutpointHasher
{
private:
    const uint64_t k0, k1;
public:
    SaltedOutpointHasher();
    size_t operator()(const COutPoint& id) const noexcept {
        return SipHashUint256Extra(k0, k1, id.hash, id.n);
    }
};
struct FilterHeaderHasher
{
    size_t operator()(const uint256& hash) const { return ReadLE64(hash.begin()); }
};
class SignatureCacheHasher
{
public:
    template <uint8_t hash_select>
    uint32_t operator()(const uint256& key) const
    {
        static_assert(hash_select <8, "SignatureCacheHasher only has 8 hashes available.");
        uint32_t u;
        std::memcpy(&u, key.begin()+4*hash_select, 4);
        return u;
    }
};
struct BlockHasher
{
    size_t operator()(const uint256& hash) const { return ReadLE64(hash.begin()); }
};
class SaltedSipHasher
{
private:
    const uint64_t m_k0, m_k1;
public:
    SaltedSipHasher();
    size_t operator()(const Span<const unsigned char>& script) const;
};
#endif
