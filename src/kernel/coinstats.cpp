#include <kernel/coinstats.h>
#include <chain.h>
#include <coins.h>
#include <crypto/muhash.h>
#include <hash.h>
#include <node/blockstorage.h>
#include <primitives/transaction.h>
#include <script/script.h>
#include <serialize.h>
#include <span.h>
#include <streams.h>
#include <sync.h>
#include <tinyformat.h>
#include <uint256.h>
#include <util/check.h>
#include <util/overflow.h>
#include <util/system.h>
#include <validation.h>
#include <version.h>
#include <cassert>
#include <iosfwd>
#include <iterator>
#include <map>
#include <memory>
#include <string>
#include <utility>
namespace kernel {
CCoinsStats::CCoinsStats(int block_height, const uint256& block_hash)
    : nHeight(block_height),
      hashBlock(block_hash) {}
uint64_t GetBogoSize(const CScript& script_pub_key)
{
    return 32   +
           4   +
           4   +
           8   +
           2   +
           script_pub_key.size()  ;
}
CDataStream TxOutSer(const COutPoint& outpoint, const Coin& coin) {
    CDataStream ss(SER_DISK, PROTOCOL_VERSION);
    ss << outpoint;
    ss << static_cast<uint32_t>(coin.nHeight * 2 + coin.fCoinBase);
    ss << coin.out;
    return ss;
}
static void ApplyHash(HashWriter& ss, const uint256& hash, const std::map<uint32_t, Coin>& outputs)
{
    for (auto it = outputs.begin(); it != outputs.end(); ++it) {
        if (it == outputs.begin()) {
            ss << hash;
            ss << VARINT(it->second.nHeight * 2 + it->second.fCoinBase ? 1u : 0u);
        }
        ss << VARINT(it->first + 1);
        ss << it->second.out.scriptPubKey;
        ss << VARINT_MODE(it->second.out.nValue, VarIntMode::NONNEGATIVE_SIGNED);
        if (it == std::prev(outputs.end())) {
            ss << VARINT(0u);
        }
    }
}
static void ApplyHash(std::nullptr_t, const uint256& hash, const std::map<uint32_t, Coin>& outputs) {}
static void ApplyHash(MuHash3072& muhash, const uint256& hash, const std::map<uint32_t, Coin>& outputs)
{
    for (auto it = outputs.begin(); it != outputs.end(); ++it) {
        COutPoint outpoint = COutPoint(hash, it->first);
        Coin coin = it->second;
        muhash.Insert(MakeUCharSpan(TxOutSer(outpoint, coin)));
    }
}
static void ApplyStats(CCoinsStats& stats, const uint256& hash, const std::map<uint32_t, Coin>& outputs)
{
    assert(!outputs.empty());
    stats.nTransactions++;
    for (auto it = outputs.begin(); it != outputs.end(); ++it) {
        stats.nTransactionOutputs++;
        if (stats.total_amount.has_value()) {
            stats.total_amount = CheckedAdd(*stats.total_amount, it->second.out.nValue);
        }
        stats.nBogoSize += GetBogoSize(it->second.out.scriptPubKey);
    }
}
template <typename T>
static bool ComputeUTXOStats(CCoinsView* view, CCoinsStats& stats, T hash_obj, const std::function<void()>& interruption_point)
{
    std::unique_ptr<CCoinsViewCursor> pcursor(view->Cursor());
    assert(pcursor);
    PrepareHash(hash_obj, stats);
    uint256 prevkey;
    std::map<uint32_t, Coin> outputs;
    while (pcursor->Valid()) {
        interruption_point();
        COutPoint key;
        Coin coin;
        if (pcursor->GetKey(key) && pcursor->GetValue(coin)) {
            if (!outputs.empty() && key.hash != prevkey) {
                ApplyStats(stats, prevkey, outputs);
                ApplyHash(hash_obj, prevkey, outputs);
                outputs.clear();
            }
            prevkey = key.hash;
            outputs[key.n] = std::move(coin);
            stats.coins_count++;
        } else {
            return error("%s: unable to read value", __func__);
        }
        pcursor->Next();
    }
    if (!outputs.empty()) {
        ApplyStats(stats, prevkey, outputs);
        ApplyHash(hash_obj, prevkey, outputs);
    }
    FinalizeHash(hash_obj, stats);
    stats.nDiskSize = view->EstimateSize();
    return true;
}
std::optional<CCoinsStats> ComputeUTXOStats(CoinStatsHashType hash_type, CCoinsView* view, node::BlockManager& blockman, const std::function<void()>& interruption_point)
{
    CBlockIndex* pindex = WITH_LOCK(::cs_main, return blockman.LookupBlockIndex(view->GetBestBlock()));
    CCoinsStats stats{Assert(pindex)->nHeight, pindex->GetBlockHash()};
    bool success = [&]() -> bool {
        switch (hash_type) {
        case(CoinStatsHashType::HASH_SERIALIZED): {
            HashWriter ss{};
            return ComputeUTXOStats(view, stats, ss, interruption_point);
        }
        case(CoinStatsHashType::MUHASH): {
            MuHash3072 muhash;
            return ComputeUTXOStats(view, stats, muhash, interruption_point);
        }
        case(CoinStatsHashType::NONE): {
            return ComputeUTXOStats(view, stats, nullptr, interruption_point);
        }
        }
        assert(false);
    }();
    if (!success) {
        return std::nullopt;
    }
    return stats;
}
static void PrepareHash(HashWriter& ss, const CCoinsStats& stats)
{
    ss << stats.hashBlock;
}
static void PrepareHash(MuHash3072& muhash, CCoinsStats& stats) {}
static void PrepareHash(std::nullptr_t, CCoinsStats& stats) {}
static void FinalizeHash(HashWriter& ss, CCoinsStats& stats)
{
    stats.hashSerialized = ss.GetHash();
}
static void FinalizeHash(MuHash3072& muhash, CCoinsStats& stats)
{
    uint256 out;
    muhash.Finalize(out);
    stats.hashSerialized = out;
}
static void FinalizeHash(std::nullptr_t, CCoinsStats& stats) {}
}
