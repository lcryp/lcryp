#ifndef LCRYP_SCRIPT_SIGCACHE_H
#define LCRYP_SCRIPT_SIGCACHE_H
#include <script/interpreter.h>
#include <span.h>
#include <util/hasher.h>
#include <optional>
#include <vector>
static constexpr size_t DEFAULT_MAX_SIG_CACHE_BYTES{32 << 20};
class CPubKey;
class CachingTransactionSignatureChecker : public TransactionSignatureChecker
{
private:
    bool store;
public:
    CachingTransactionSignatureChecker(const CTransaction* txToIn, unsigned int nInIn, const CAmount& amountIn, bool storeIn, PrecomputedTransactionData& txdataIn) : TransactionSignatureChecker(txToIn, nInIn, amountIn, txdataIn, MissingDataBehavior::ASSERT_FAIL), store(storeIn) {}
    bool VerifyECDSASignature(const std::vector<unsigned char>& vchSig, const CPubKey& vchPubKey, const uint256& sighash) const override;
    bool VerifySchnorrSignature(Span<const unsigned char> sig, const XOnlyPubKey& pubkey, const uint256& sighash) const override;
};
[[nodiscard]] bool InitSignatureCache(size_t max_size_bytes);
#endif
