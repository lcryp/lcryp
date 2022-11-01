#ifndef LCRYP_HEADERSSYNC_H
#define LCRYP_HEADERSSYNC_H
#include <arith_uint256.h>
#include <chain.h>
#include <consensus/params.h>
#include <net.h>
#include <primitives/block.h>
#include <uint256.h>
#include <util/bitdeque.h>
#include <util/hasher.h>
#include <deque>
#include <vector>
struct CompressedHeader {
    int32_t nVersion{0};
    uint256 hashMerkleRoot;
    uint32_t nTime{0};
    uint32_t nBits{0};
    uint32_t nNonce{0};
    CompressedHeader()
    {
        hashMerkleRoot.SetNull();
    }
    CompressedHeader(const CBlockHeader& header)
    {
        nVersion = header.nVersion;
        hashMerkleRoot = header.hashMerkleRoot;
        nTime = header.nTime;
        nBits = header.nBits;
        nNonce = header.nNonce;
    }
    CBlockHeader GetFullHeader(const uint256& hash_prev_block) {
        CBlockHeader ret;
        ret.nVersion = nVersion;
        ret.hashPrevBlock = hash_prev_block;
        ret.hashMerkleRoot = hashMerkleRoot;
        ret.nTime = nTime;
        ret.nBits = nBits;
        ret.nNonce = nNonce;
        return ret;
    };
};
class HeadersSyncState {
public:
    ~HeadersSyncState() {}
    enum class State {
        PRESYNC,
        REDOWNLOAD,
        FINAL
    };
    State GetState() const { return m_download_state; }
    int64_t GetPresyncHeight() const { return m_current_height; }
    uint32_t GetPresyncTime() const { return m_last_header_received.nTime; }
    arith_uint256 GetPresyncWork() const { return m_current_chain_work; }
    HeadersSyncState(NodeId id, const Consensus::Params& consensus_params,
            const CBlockIndex* chain_start, const arith_uint256& minimum_required_work);
    struct ProcessingResult {
        std::vector<CBlockHeader> pow_validated_headers;
        bool success{false};
        bool request_more{false};
    };
    ProcessingResult ProcessNextHeaders(const std::vector<CBlockHeader>&
            received_headers, bool full_headers_message);
    CBlockLocator NextHeadersRequestLocator() const;
private:
    void Finalize();
    bool ValidateAndStoreHeadersCommitments(const std::vector<CBlockHeader>& headers);
    bool ValidateAndProcessSingleHeader(const CBlockHeader& current);
    bool ValidateAndStoreRedownloadedHeader(const CBlockHeader& header);
    std::vector<CBlockHeader> PopHeadersReadyForAcceptance();
private:
    const NodeId m_id;
    const Consensus::Params& m_consensus_params;
    const CBlockIndex* m_chain_start{nullptr};
    const arith_uint256 m_minimum_required_work;
    arith_uint256 m_current_chain_work;
    const SaltedTxidHasher m_hasher;
    bitdeque<> m_header_commitments;
    const unsigned m_commit_offset;
    uint64_t m_max_commitments{0};
    CBlockHeader m_last_header_received;
    int64_t m_current_height{0};
    std::deque<CompressedHeader> m_redownloaded_headers;
    int64_t m_redownload_buffer_last_height{0};
    uint256 m_redownload_buffer_last_hash;
    uint256 m_redownload_buffer_first_prev_hash;
    arith_uint256 m_redownload_chain_work;
    bool m_process_all_remaining_headers{false};
    State m_download_state{State::PRESYNC};
};
#endif
