#include <script/sigcache.h>
#include <pubkey.h>
#include <random.h>
#include <uint256.h>
#include <util/system.h>
#include <cuckoocache.h>
#include <algorithm>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <vector>
namespace {
class CSignatureCache
{
private:
    CSHA256 m_salted_hasher_ecdsa;
    CSHA256 m_salted_hasher_schnorr;
    typedef CuckooCache::cache<uint256, SignatureCacheHasher> map_type;
    map_type setValid;
    std::shared_mutex cs_sigcache;
public:
    CSignatureCache()
    {
        uint256 nonce = GetRandHash();
        static constexpr unsigned char PADDING_ECDSA[32] = {'E'};
        static constexpr unsigned char PADDING_SCHNORR[32] = {'S'};
        m_salted_hasher_ecdsa.Write(nonce.begin(), 32);
        m_salted_hasher_ecdsa.Write(PADDING_ECDSA, 32);
        m_salted_hasher_schnorr.Write(nonce.begin(), 32);
        m_salted_hasher_schnorr.Write(PADDING_SCHNORR, 32);
    }
    void
    ComputeEntryECDSA(uint256& entry, const uint256 &hash, const std::vector<unsigned char>& vchSig, const CPubKey& pubkey) const
    {
        CSHA256 hasher = m_salted_hasher_ecdsa;
        hasher.Write(hash.begin(), 32).Write(pubkey.data(), pubkey.size()).Write(vchSig.data(), vchSig.size()).Finalize(entry.begin());
    }
    void
    ComputeEntrySchnorr(uint256& entry, const uint256 &hash, Span<const unsigned char> sig, const XOnlyPubKey& pubkey) const
    {
        CSHA256 hasher = m_salted_hasher_schnorr;
        hasher.Write(hash.begin(), 32).Write(pubkey.data(), pubkey.size()).Write(sig.data(), sig.size()).Finalize(entry.begin());
    }
    bool
    Get(const uint256& entry, const bool erase)
    {
        std::shared_lock<std::shared_mutex> lock(cs_sigcache);
        return setValid.contains(entry, erase);
    }
    void Set(const uint256& entry)
    {
        std::unique_lock<std::shared_mutex> lock(cs_sigcache);
        setValid.insert(entry);
    }
    std::optional<std::pair<uint32_t, size_t>> setup_bytes(size_t n)
    {
        return setValid.setup_bytes(n);
    }
};
static CSignatureCache signatureCache;
}
bool InitSignatureCache(size_t max_size_bytes)
{
    auto setup_results = signatureCache.setup_bytes(max_size_bytes);
    if (!setup_results) return false;
    const auto [num_elems, approx_size_bytes] = *setup_results;
    LogPrintf("Using %zu MiB out of %zu MiB requested for signature cache, able to store %zu elements\n",
              approx_size_bytes >> 20, max_size_bytes >> 20, num_elems);
    return true;
}
bool CachingTransactionSignatureChecker::VerifyECDSASignature(const std::vector<unsigned char>& vchSig, const CPubKey& pubkey, const uint256& sighash) const
{
    uint256 entry;
    signatureCache.ComputeEntryECDSA(entry, sighash, vchSig, pubkey);
    if (signatureCache.Get(entry, !store))
        return true;
    if (!TransactionSignatureChecker::VerifyECDSASignature(vchSig, pubkey, sighash))
        return false;
    if (store)
        signatureCache.Set(entry);
    return true;
}
bool CachingTransactionSignatureChecker::VerifySchnorrSignature(Span<const unsigned char> sig, const XOnlyPubKey& pubkey, const uint256& sighash) const
{
    uint256 entry;
    signatureCache.ComputeEntrySchnorr(entry, sighash, sig, pubkey);
    if (signatureCache.Get(entry, !store)) return true;
    if (!TransactionSignatureChecker::VerifySchnorrSignature(sig, pubkey, sighash)) return false;
    if (store) signatureCache.Set(entry);
    return true;
}
