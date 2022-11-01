#ifndef LCRYP_VERSIONBITS_H
#define LCRYP_VERSIONBITS_H
#include <chain.h>
#include <sync.h>
#include <map>
static const int32_t VERSIONBITS_LAST_OLD_BLOCK_VERSION = 4;
static const int32_t VERSIONBITS_TOP_BITS = 0x20000000UL;
static const int32_t VERSIONBITS_TOP_MASK = 0xE0000000UL;
static const int32_t VERSIONBITS_NUM_BITS = 29;
enum class ThresholdState {
    DEFINED,
    STARTED,
    LOCKED_IN,
    ACTIVE,
    FAILED,
};
typedef std::map<const CBlockIndex*, ThresholdState> ThresholdConditionCache;
struct BIP9Stats {
    int period;
    int threshold;
    int elapsed;
    int count;
    bool possible;
};
class AbstractThresholdConditionChecker {
protected:
    virtual bool Condition(const CBlockIndex* pindex, const Consensus::Params& params) const =0;
    virtual int64_t BeginTime(const Consensus::Params& params) const =0;
    virtual int64_t EndTime(const Consensus::Params& params) const =0;
    virtual int MinActivationHeight(const Consensus::Params& params) const { return 0; }
    virtual int Period(const Consensus::Params& params) const =0;
    virtual int Threshold(const Consensus::Params& params) const =0;
public:
    BIP9Stats GetStateStatisticsFor(const CBlockIndex* pindex, const Consensus::Params& params, std::vector<bool>* signalling_blocks = nullptr) const;
    ThresholdState GetStateFor(const CBlockIndex* pindexPrev, const Consensus::Params& params, ThresholdConditionCache& cache) const;
    int GetStateSinceHeightFor(const CBlockIndex* pindexPrev, const Consensus::Params& params, ThresholdConditionCache& cache) const;
};
class VersionBitsCache
{
private:
    Mutex m_mutex;
    ThresholdConditionCache m_caches[Consensus::MAX_VERSION_BITS_DEPLOYMENTS] GUARDED_BY(m_mutex);
public:
    static BIP9Stats Statistics(const CBlockIndex* pindex, const Consensus::Params& params, Consensus::DeploymentPos pos, std::vector<bool>* signalling_blocks = nullptr);
    static uint32_t Mask(const Consensus::Params& params, Consensus::DeploymentPos pos);
    ThresholdState State(const CBlockIndex* pindexPrev, const Consensus::Params& params, Consensus::DeploymentPos pos) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);
    int StateSinceHeight(const CBlockIndex* pindexPrev, const Consensus::Params& params, Consensus::DeploymentPos pos) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);
    int32_t ComputeBlockVersion(const CBlockIndex* pindexPrev, const Consensus::Params& params) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);
    void Clear() EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);
};
#endif
