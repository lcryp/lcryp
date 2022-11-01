#include <headerssync.h>
#include <logging.h>
#include <pow.h>
#include <timedata.h>
#include <util/check.h>
constexpr size_t HEADER_COMMITMENT_PERIOD{584};
constexpr size_t REDOWNLOAD_BUFFER_SIZE{13959};
static_assert(sizeof(CompressedHeader) == 48);
HeadersSyncState::HeadersSyncState(NodeId id, const Consensus::Params& consensus_params,
        const CBlockIndex* chain_start, const arith_uint256& minimum_required_work) :
    m_id(id), m_consensus_params(consensus_params),
    m_chain_start(chain_start),
    m_minimum_required_work(minimum_required_work),
    m_current_chain_work(chain_start->nChainWork),
    m_commit_offset(GetRand<unsigned>(HEADER_COMMITMENT_PERIOD)),
    m_last_header_received(m_chain_start->GetBlockHeader()),
    m_current_height(chain_start->nHeight)
{
    m_max_commitments = 6*(Ticks<std::chrono::seconds>(GetAdjustedTime() - NodeSeconds{std::chrono::seconds{chain_start->GetMedianTimePast()}}) + MAX_FUTURE_BLOCK_TIME) / HEADER_COMMITMENT_PERIOD;
    LogPrint(BCLog::NET, "Initial headers sync started with peer=%d: height=%i, max_commitments=%i, min_work=%s\n", m_id, m_current_height, m_max_commitments, m_minimum_required_work.ToString());
}
void HeadersSyncState::Finalize()
{
    Assume(m_download_state != State::FINAL);
    m_header_commitments = {};
    m_last_header_received.SetNull();
    m_redownloaded_headers = {};
    m_redownload_buffer_last_hash.SetNull();
    m_redownload_buffer_first_prev_hash.SetNull();
    m_process_all_remaining_headers = false;
    m_current_height = 0;
    m_download_state = State::FINAL;
}
HeadersSyncState::ProcessingResult HeadersSyncState::ProcessNextHeaders(const
        std::vector<CBlockHeader>& received_headers, const bool full_headers_message)
{
    ProcessingResult ret;
    Assume(!received_headers.empty());
    if (received_headers.empty()) return ret;
    Assume(m_download_state != State::FINAL);
    if (m_download_state == State::FINAL) return ret;
    if (m_download_state == State::PRESYNC) {
        ret.success = ValidateAndStoreHeadersCommitments(received_headers);
        if (ret.success) {
            if (full_headers_message || m_download_state == State::REDOWNLOAD) {
                ret.request_more = true;
            } else {
                Assume(m_download_state == State::PRESYNC);
                LogPrint(BCLog::NET, "Initial headers sync aborted with peer=%d: incomplete headers message at height=%i (presync phase)\n", m_id, m_current_height);
            }
        }
    } else if (m_download_state == State::REDOWNLOAD) {
        ret.success = true;
        for (const auto& hdr : received_headers) {
            if (!ValidateAndStoreRedownloadedHeader(hdr)) {
                ret.success = false;
                break;
            }
        }
        if (ret.success) {
            ret.pow_validated_headers = PopHeadersReadyForAcceptance();
            if (m_redownloaded_headers.empty() && m_process_all_remaining_headers) {
                LogPrint(BCLog::NET, "Initial headers sync complete with peer=%d: releasing all at height=%i (redownload phase)\n", m_id, m_redownload_buffer_last_height);
            } else if (full_headers_message) {
                ret.request_more = true;
            } else {
                LogPrint(BCLog::NET, "Initial headers sync aborted with peer=%d: incomplete headers message at height=%i (redownload phase)\n", m_id, m_redownload_buffer_last_height);
            }
        }
    }
    if (!(ret.success && ret.request_more)) Finalize();
    return ret;
}
bool HeadersSyncState::ValidateAndStoreHeadersCommitments(const std::vector<CBlockHeader>& headers)
{
    Assume(headers.size() > 0);
    if (headers.size() == 0) return true;
    Assume(m_download_state == State::PRESYNC);
    if (m_download_state != State::PRESYNC) return false;
    if (headers[0].hashPrevBlock != m_last_header_received.GetHash()) {
        LogPrint(BCLog::NET, "Initial headers sync aborted with peer=%d: non-continuous headers at height=%i (presync phase)\n", m_id, m_current_height);
        return false;
    }
    for (const auto& hdr : headers) {
        if (!ValidateAndProcessSingleHeader(hdr)) {
            return false;
        }
    }
    if (m_current_chain_work >= m_minimum_required_work) {
        m_redownloaded_headers.clear();
        m_redownload_buffer_last_height = m_chain_start->nHeight;
        m_redownload_buffer_first_prev_hash = m_chain_start->GetBlockHash();
        m_redownload_buffer_last_hash = m_chain_start->GetBlockHash();
        m_redownload_chain_work = m_chain_start->nChainWork;
        m_download_state = State::REDOWNLOAD;
        LogPrint(BCLog::NET, "Initial headers sync transition with peer=%d: reached sufficient work at height=%i, redownloading from height=%i\n", m_id, m_current_height, m_redownload_buffer_last_height);
    }
    return true;
}
bool HeadersSyncState::ValidateAndProcessSingleHeader(const CBlockHeader& current)
{
    Assume(m_download_state == State::PRESYNC);
    if (m_download_state != State::PRESYNC) return false;
    int next_height = m_current_height + 1;
    if (!PermittedDifficultyTransition(m_consensus_params, next_height,
                m_last_header_received.nBits, current.nBits)) {
        LogPrint(BCLog::NET, "Initial headers sync aborted with peer=%d: invalid difficulty transition at height=%i (presync phase)\n", m_id, next_height);
        return false;
    }
    if (next_height % HEADER_COMMITMENT_PERIOD == m_commit_offset) {
        m_header_commitments.push_back(m_hasher(current.GetHash()) & 1);
        if (m_header_commitments.size() > m_max_commitments) {
            LogPrint(BCLog::NET, "Initial headers sync aborted with peer=%d: exceeded max commitments at height=%i (presync phase)\n", m_id, next_height);
            return false;
        }
    }
    m_current_chain_work += GetBlockProof(CBlockIndex(current));
    m_last_header_received = current;
    m_current_height = next_height;
    return true;
}
bool HeadersSyncState::ValidateAndStoreRedownloadedHeader(const CBlockHeader& header)
{
    Assume(m_download_state == State::REDOWNLOAD);
    if (m_download_state != State::REDOWNLOAD) return false;
    int64_t next_height = m_redownload_buffer_last_height + 1;
    if (header.hashPrevBlock != m_redownload_buffer_last_hash) {
        LogPrint(BCLog::NET, "Initial headers sync aborted with peer=%d: non-continuous headers at height=%i (redownload phase)\n", m_id, next_height);
        return false;
    }
    uint32_t previous_nBits{0};
    if (!m_redownloaded_headers.empty()) {
        previous_nBits = m_redownloaded_headers.back().nBits;
    } else {
        previous_nBits = m_chain_start->nBits;
    }
    if (!PermittedDifficultyTransition(m_consensus_params, next_height,
                previous_nBits, header.nBits)) {
        LogPrint(BCLog::NET, "Initial headers sync aborted with peer=%d: invalid difficulty transition at height=%i (redownload phase)\n", m_id, next_height);
        return false;
    }
    m_redownload_chain_work += GetBlockProof(CBlockIndex(header));
    if (m_redownload_chain_work >= m_minimum_required_work) {
        m_process_all_remaining_headers = true;
    }
    if (!m_process_all_remaining_headers && next_height % HEADER_COMMITMENT_PERIOD == m_commit_offset) {
        if (m_header_commitments.size() == 0) {
            LogPrint(BCLog::NET, "Initial headers sync aborted with peer=%d: commitment overrun at height=%i (redownload phase)\n", m_id, next_height);
            return false;
        }
        bool commitment = m_hasher(header.GetHash()) & 1;
        bool expected_commitment = m_header_commitments.front();
        m_header_commitments.pop_front();
        if (commitment != expected_commitment) {
            LogPrint(BCLog::NET, "Initial headers sync aborted with peer=%d: commitment mismatch at height=%i (redownload phase)\n", m_id, next_height);
            return false;
        }
    }
    m_redownloaded_headers.push_back(header);
    m_redownload_buffer_last_height = next_height;
    m_redownload_buffer_last_hash = header.GetHash();
    return true;
}
std::vector<CBlockHeader> HeadersSyncState::PopHeadersReadyForAcceptance()
{
    std::vector<CBlockHeader> ret;
    Assume(m_download_state == State::REDOWNLOAD);
    if (m_download_state != State::REDOWNLOAD) return ret;
    while (m_redownloaded_headers.size() > REDOWNLOAD_BUFFER_SIZE ||
            (m_redownloaded_headers.size() > 0 && m_process_all_remaining_headers)) {
        ret.emplace_back(m_redownloaded_headers.front().GetFullHeader(m_redownload_buffer_first_prev_hash));
        m_redownloaded_headers.pop_front();
        m_redownload_buffer_first_prev_hash = ret.back().GetHash();
    }
    return ret;
}
CBlockLocator HeadersSyncState::NextHeadersRequestLocator() const
{
    Assume(m_download_state != State::FINAL);
    if (m_download_state == State::FINAL) return {};
    auto chain_start_locator = LocatorEntries(m_chain_start);
    std::vector<uint256> locator;
    if (m_download_state == State::PRESYNC) {
        locator.push_back(m_last_header_received.GetHash());
    }
    if (m_download_state == State::REDOWNLOAD) {
        locator.push_back(m_redownload_buffer_last_hash);
    }
    locator.insert(locator.end(), chain_start_locator.begin(), chain_start_locator.end());
    return CBlockLocator{std::move(locator)};
}
