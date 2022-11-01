#ifndef LCRYP_NODE_UTXO_SNAPSHOT_H
#define LCRYP_NODE_UTXO_SNAPSHOT_H
#include <uint256.h>
#include <serialize.h>
namespace node {
class SnapshotMetadata
{
public:
    uint256 m_base_blockhash;
    uint64_t m_coins_count = 0;
    SnapshotMetadata() { }
    SnapshotMetadata(
        const uint256& base_blockhash,
        uint64_t coins_count,
        unsigned int nchaintx) :
            m_base_blockhash(base_blockhash),
            m_coins_count(coins_count) { }
    SERIALIZE_METHODS(SnapshotMetadata, obj) { READWRITE(obj.m_base_blockhash, obj.m_coins_count); }
};
}
#endif
