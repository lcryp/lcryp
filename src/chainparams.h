#ifndef LCRYP_CHAINPARAMS_H
#define LCRYP_CHAINPARAMS_H
#include <chainparamsbase.h>
#include <consensus/params.h>
#include <netaddress.h>
#include <primitives/block.h>
#include <protocol.h>
#include <util/hash_type.h>
#include <memory>
#include <string>
#include <vector>
typedef std::map<int, uint256> MapCheckpoints;
struct CCheckpointData {
    MapCheckpoints mapCheckpoints;
    int GetHeight() const {
        const auto& final_checkpoint = mapCheckpoints.rbegin();
        return final_checkpoint->first  ;
    }
};
struct AssumeutxoHash : public BaseHash<uint256> {
    explicit AssumeutxoHash(const uint256& hash) : BaseHash(hash) {}
};
struct AssumeutxoData {
    const AssumeutxoHash hash_serialized;
    const unsigned int nChainTx;
};
using MapAssumeutxo = std::map<int, const AssumeutxoData>;
struct ChainTxData {
    int64_t nTime;
    int64_t nTxCount;
    double dTxRate;
};
class CChainParams
{
public:
    enum Base58Type {
        PUBKEY_ADDRESS,
        SCRIPT_ADDRESS,
        SECRET_KEY,
        EXT_PUBLIC_KEY,
        EXT_SECRET_KEY,
        MAX_BASE58_TYPES
    };
    const Consensus::Params& GetConsensus() const { return consensus; }
    const CMessageHeader::MessageStartChars& MessageStart() const { return pchMessageStart; }
    uint16_t GetDefaultPort() const { return nDefaultPort; }
    uint16_t GetDefaultPort(Network net) const
    {
        return net == NET_I2P ? I2P_SAM31_PORT : GetDefaultPort();
    }
    uint16_t GetDefaultPort(const std::string& addr) const
    {
        CNetAddr a;
        return a.SetSpecial(addr) ? GetDefaultPort(a.GetNetwork()) : GetDefaultPort();
    }
    const CBlock& GenesisBlock() const { return genesis; }
    bool DefaultConsistencyChecks() const { return fDefaultConsistencyChecks; }
    bool RequireStandard() const { return fRequireStandard; }
    bool IsTestChain() const { return m_is_test_chain; }
    bool IsMockableChain() const { return m_is_mockable_chain; }
    uint64_t PruneAfterHeight() const { return nPruneAfterHeight; }
    uint64_t AssumedBlockchainSize() const { return m_assumed_blockchain_size; }
    uint64_t AssumedChainStateSize() const { return m_assumed_chain_state_size; }
    bool MineBlocksOnDemand() const { return consensus.fPowNoRetargeting; }
    std::string NetworkIDString() const { return strNetworkID; }
    const std::vector<std::string>& DNSSeeds() const { return vSeeds; }
    const std::vector<unsigned char>& Base58Prefix(Base58Type type) const { return base58Prefixes[type]; }
    const std::string& Bech32HRP() const { return bech32_hrp; }
    const std::vector<uint8_t>& FixedSeeds() const { return vFixedSeeds; }
    const CCheckpointData& Checkpoints() const { return checkpointData; }
    const MapAssumeutxo& Assumeutxo() const { return m_assumeutxo_data; }
    const ChainTxData& TxData() const { return chainTxData; }
protected:
    CChainParams() {}
    Consensus::Params consensus;
    CMessageHeader::MessageStartChars pchMessageStart;
    uint16_t nDefaultPort;
    uint64_t nPruneAfterHeight;
    uint64_t m_assumed_blockchain_size;
    uint64_t m_assumed_chain_state_size;
    std::vector<std::string> vSeeds;
    std::vector<unsigned char> base58Prefixes[MAX_BASE58_TYPES];
    std::string bech32_hrp;
    std::string strNetworkID;
    CBlock genesis;
    std::vector<uint8_t> vFixedSeeds;
    bool fDefaultConsistencyChecks;
    bool fRequireStandard;
    bool m_is_test_chain;
    bool m_is_mockable_chain;
    CCheckpointData checkpointData;
    MapAssumeutxo m_assumeutxo_data;
    ChainTxData chainTxData;
};
std::unique_ptr<const CChainParams> CreateChainParams(const ArgsManager& args, const std::string& chain);
const CChainParams &Params();
void SelectParams(const std::string& chain);
#endif
