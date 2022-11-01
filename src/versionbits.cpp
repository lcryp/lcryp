#include <consensus/params.h>
#include <util/check.h>
#include <versionbits.h>
ThresholdState AbstractThresholdConditionChecker::GetStateFor(const CBlockIndex* pindexPrev, const Consensus::Params& params, ThresholdConditionCache& cache) const
{
    int nPeriod = Period(params);
    int nThreshold = Threshold(params);
    int min_activation_height = MinActivationHeight(params);
    int64_t nTimeStart = BeginTime(params);
    int64_t nTimeTimeout = EndTime(params);
    if (nTimeStart == Consensus::BIP9Deployment::ALWAYS_ACTIVE) {
        return ThresholdState::ACTIVE;
    }
    if (nTimeStart == Consensus::BIP9Deployment::NEVER_ACTIVE) {
        return ThresholdState::FAILED;
    }
    if (pindexPrev != nullptr) {
        pindexPrev = pindexPrev->GetAncestor(pindexPrev->nHeight - ((pindexPrev->nHeight + 1) % nPeriod));
    }
    std::vector<const CBlockIndex*> vToCompute;
    while (cache.count(pindexPrev) == 0) {
        if (pindexPrev == nullptr) {
            cache[pindexPrev] = ThresholdState::DEFINED;
            break;
        }
        if (pindexPrev->GetMedianTimePast() < nTimeStart) {
            cache[pindexPrev] = ThresholdState::DEFINED;
            break;
        }
        vToCompute.push_back(pindexPrev);
        pindexPrev = pindexPrev->GetAncestor(pindexPrev->nHeight - nPeriod);
    }
    assert(cache.count(pindexPrev));
    ThresholdState state = cache[pindexPrev];
    while (!vToCompute.empty()) {
        ThresholdState stateNext = state;
        pindexPrev = vToCompute.back();
        vToCompute.pop_back();
        switch (state) {
            case ThresholdState::DEFINED: {
                if (pindexPrev->GetMedianTimePast() >= nTimeStart) {
                    stateNext = ThresholdState::STARTED;
                }
                break;
            }
            case ThresholdState::STARTED: {
                const CBlockIndex* pindexCount = pindexPrev;
                int count = 0;
                for (int i = 0; i < nPeriod; i++) {
                    if (Condition(pindexCount, params)) {
                        count++;
                    }
                    pindexCount = pindexCount->pprev;
                }
                if (count >= nThreshold && count>0) {
                    stateNext = ThresholdState::LOCKED_IN;
                } else if (pindexPrev->GetMedianTimePast() >= nTimeTimeout) {
                    stateNext = ThresholdState::FAILED;
                }
                break;
            }
            case ThresholdState::LOCKED_IN: {
                if (pindexPrev->nHeight + 1 >= min_activation_height) {
                    stateNext = ThresholdState::ACTIVE;
                }
                break;
            }
            case ThresholdState::FAILED:
            case ThresholdState::ACTIVE: {
                break;
            }
        }
        cache[pindexPrev] = state = stateNext;
    }
    return state;
}
BIP9Stats AbstractThresholdConditionChecker::GetStateStatisticsFor(const CBlockIndex* pindex, const Consensus::Params& params, std::vector<bool>* signalling_blocks) const
{
    BIP9Stats stats = {};
    stats.period = Period(params);
    stats.threshold = Threshold(params);
    if (pindex == nullptr) return stats;
    int blocks_in_period = 1 + (pindex->nHeight % stats.period);
    if (signalling_blocks) {
        signalling_blocks->assign(blocks_in_period, false);
    }
    int elapsed = 0;
    int count = 0;
    const CBlockIndex* currentIndex = pindex;
    do {
        ++elapsed;
        --blocks_in_period;
        if (Condition(currentIndex, params)) {
            ++count;
            if (signalling_blocks) signalling_blocks->at(blocks_in_period) = true;
        }
        currentIndex = currentIndex->pprev;
    } while(blocks_in_period > 0);
    stats.elapsed = elapsed;
    stats.count = count;
    stats.possible = (stats.period - stats.threshold ) >= (stats.elapsed - count);
    return stats;
}
int AbstractThresholdConditionChecker::GetStateSinceHeightFor(const CBlockIndex* pindexPrev, const Consensus::Params& params, ThresholdConditionCache& cache) const
{
    int64_t start_time = BeginTime(params);
    if (start_time == Consensus::BIP9Deployment::ALWAYS_ACTIVE || start_time == Consensus::BIP9Deployment::NEVER_ACTIVE) {
        return 0;
    }
    const ThresholdState initialState = GetStateFor(pindexPrev, params, cache);
    if (initialState == ThresholdState::DEFINED) {
        return 0;
    }
    const int nPeriod = Period(params);
    pindexPrev = Assert(pindexPrev->GetAncestor(pindexPrev->nHeight - ((pindexPrev->nHeight + 1) % nPeriod)));
    const CBlockIndex* previousPeriodParent = pindexPrev->GetAncestor(pindexPrev->nHeight - nPeriod);
    while (previousPeriodParent != nullptr && GetStateFor(previousPeriodParent, params, cache) == initialState) {
        pindexPrev = previousPeriodParent;
        previousPeriodParent = pindexPrev->GetAncestor(pindexPrev->nHeight - nPeriod);
    }
    return pindexPrev->nHeight + 1;
}
namespace
{
class VersionBitsConditionChecker : public AbstractThresholdConditionChecker {
private:
    const Consensus::DeploymentPos id;
protected:
    int64_t BeginTime(const Consensus::Params& params) const override { return params.vDeployments[id].nStartTime; }
    int64_t EndTime(const Consensus::Params& params) const override { return params.vDeployments[id].nTimeout; }
    int MinActivationHeight(const Consensus::Params& params) const override { return params.vDeployments[id].min_activation_height; }
    int Period(const Consensus::Params& params) const override { return params.nMinerConfirmationWindow; }
    int Threshold(const Consensus::Params& params) const override { return params.nRuleChangeActivationThreshold; }
    bool Condition(const CBlockIndex* pindex, const Consensus::Params& params) const override
    {
        return (((pindex->nVersion & VERSIONBITS_TOP_MASK) == VERSIONBITS_TOP_BITS) && (pindex->nVersion & Mask(params)) != 0);
    }
public:
    explicit VersionBitsConditionChecker(Consensus::DeploymentPos id_) : id(id_) {}
    uint32_t Mask(const Consensus::Params& params) const { return ((uint32_t)1) << params.vDeployments[id].bit; }
};
}
ThresholdState VersionBitsCache::State(const CBlockIndex* pindexPrev, const Consensus::Params& params, Consensus::DeploymentPos pos)
{
    LOCK(m_mutex);
    return VersionBitsConditionChecker(pos).GetStateFor(pindexPrev, params, m_caches[pos]);
}
BIP9Stats VersionBitsCache::Statistics(const CBlockIndex* pindex, const Consensus::Params& params, Consensus::DeploymentPos pos, std::vector<bool>* signalling_blocks)
{
    return VersionBitsConditionChecker(pos).GetStateStatisticsFor(pindex, params, signalling_blocks);
}
int VersionBitsCache::StateSinceHeight(const CBlockIndex* pindexPrev, const Consensus::Params& params, Consensus::DeploymentPos pos)
{
    LOCK(m_mutex);
    return VersionBitsConditionChecker(pos).GetStateSinceHeightFor(pindexPrev, params, m_caches[pos]);
}
uint32_t VersionBitsCache::Mask(const Consensus::Params& params, Consensus::DeploymentPos pos)
{
    return VersionBitsConditionChecker(pos).Mask(params);
}
int32_t VersionBitsCache::ComputeBlockVersion(const CBlockIndex* pindexPrev, const Consensus::Params& params)
{
    LOCK(m_mutex);
    int32_t nVersion = VERSIONBITS_TOP_BITS;
    for (int i = 0; i < (int)Consensus::MAX_VERSION_BITS_DEPLOYMENTS; i++) {
        Consensus::DeploymentPos pos = static_cast<Consensus::DeploymentPos>(i);
        ThresholdState state = VersionBitsConditionChecker(pos).GetStateFor(pindexPrev, params, m_caches[pos]);
        if (state == ThresholdState::LOCKED_IN || state == ThresholdState::STARTED) {
            nVersion |= Mask(params, pos);
        }
    }
    return nVersion;
}
void VersionBitsCache::Clear()
{
    LOCK(m_mutex);
    for (unsigned int d = 0; d < Consensus::MAX_VERSION_BITS_DEPLOYMENTS; d++) {
        m_caches[d].clear();
    }
}
