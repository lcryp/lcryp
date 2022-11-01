#ifndef LCRYP_POLICY_FEES_H
#define LCRYP_POLICY_FEES_H
#include <consensus/amount.h>
#include <fs.h>
#include <policy/feerate.h>
#include <random.h>
#include <sync.h>
#include <threadsafety.h>
#include <uint256.h>
#include <array>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>
class AutoFile;
class CTxMemPoolEntry;
class TxConfirmStats;
enum class FeeEstimateHorizon {
    SHORT_HALFLIFE,
    MED_HALFLIFE,
    LONG_HALFLIFE,
};
static constexpr auto ALL_FEE_ESTIMATE_HORIZONS = std::array{
    FeeEstimateHorizon::SHORT_HALFLIFE,
    FeeEstimateHorizon::MED_HALFLIFE,
    FeeEstimateHorizon::LONG_HALFLIFE,
};
std::string StringForFeeEstimateHorizon(FeeEstimateHorizon horizon);
enum class FeeReason {
    NONE,
    HALF_ESTIMATE,
    FULL_ESTIMATE,
    DOUBLE_ESTIMATE,
    CONSERVATIVE,
    MEMPOOL_MIN,
    PAYTXFEE,
    FALLBACK,
    REQUIRED,
};
struct EstimatorBucket
{
    double start = -1;
    double end = -1;
    double withinTarget = 0;
    double totalConfirmed = 0;
    double inMempool = 0;
    double leftMempool = 0;
};
struct EstimationResult
{
    EstimatorBucket pass;
    EstimatorBucket fail;
    double decay = 0;
    unsigned int scale = 0;
};
struct FeeCalculation
{
    EstimationResult est;
    FeeReason reason = FeeReason::NONE;
    int desiredTarget = 0;
    int returnedTarget = 0;
};
class CBlockPolicyEstimator
{
private:
    static constexpr unsigned int SHORT_BLOCK_PERIODS = 12;
    static constexpr unsigned int SHORT_SCALE = 1;
    static constexpr unsigned int MED_BLOCK_PERIODS = 24;
    static constexpr unsigned int MED_SCALE = 2;
    static constexpr unsigned int LONG_BLOCK_PERIODS = 42;
    static constexpr unsigned int LONG_SCALE = 24;
    static const unsigned int OLDEST_ESTIMATE_HISTORY = 6 * 1008;
    static constexpr double SHORT_DECAY = .962;
    static constexpr double MED_DECAY = .9952;
    static constexpr double LONG_DECAY = .99931;
    static constexpr double HALF_SUCCESS_PCT = .6;
    static constexpr double SUCCESS_PCT = .85;
    static constexpr double DOUBLE_SUCCESS_PCT = .95;
    static constexpr double SUFFICIENT_FEETXS = 0.1;
    static constexpr double SUFFICIENT_TXS_SHORT = 0.5;
    static constexpr double MIN_BUCKET_FEERATE = 1000;
    static constexpr double MAX_BUCKET_FEERATE = 1e7;
    static constexpr double FEE_SPACING = 1.05;
    const fs::path m_estimation_filepath;
public:
    CBlockPolicyEstimator(const fs::path& estimation_filepath);
    ~CBlockPolicyEstimator();
    void processBlock(unsigned int nBlockHeight,
                      std::vector<const CTxMemPoolEntry*>& entries)
        EXCLUSIVE_LOCKS_REQUIRED(!m_cs_fee_estimator);
    void processTransaction(const CTxMemPoolEntry& entry, bool validFeeEstimate)
        EXCLUSIVE_LOCKS_REQUIRED(!m_cs_fee_estimator);
    bool removeTx(uint256 hash, bool inBlock)
        EXCLUSIVE_LOCKS_REQUIRED(!m_cs_fee_estimator);
    CFeeRate estimateFee(int confTarget) const
        EXCLUSIVE_LOCKS_REQUIRED(!m_cs_fee_estimator);
    CFeeRate estimateSmartFee(int confTarget, FeeCalculation *feeCalc, bool conservative) const
        EXCLUSIVE_LOCKS_REQUIRED(!m_cs_fee_estimator);
    CFeeRate estimateRawFee(int confTarget, double successThreshold, FeeEstimateHorizon horizon,
                            EstimationResult* result = nullptr) const
        EXCLUSIVE_LOCKS_REQUIRED(!m_cs_fee_estimator);
    bool Write(AutoFile& fileout) const
        EXCLUSIVE_LOCKS_REQUIRED(!m_cs_fee_estimator);
    bool Read(AutoFile& filein)
        EXCLUSIVE_LOCKS_REQUIRED(!m_cs_fee_estimator);
    void FlushUnconfirmed()
        EXCLUSIVE_LOCKS_REQUIRED(!m_cs_fee_estimator);
    unsigned int HighestTargetTracked(FeeEstimateHorizon horizon) const
        EXCLUSIVE_LOCKS_REQUIRED(!m_cs_fee_estimator);
    void Flush()
        EXCLUSIVE_LOCKS_REQUIRED(!m_cs_fee_estimator);
private:
    mutable Mutex m_cs_fee_estimator;
    unsigned int nBestSeenHeight GUARDED_BY(m_cs_fee_estimator);
    unsigned int firstRecordedHeight GUARDED_BY(m_cs_fee_estimator);
    unsigned int historicalFirst GUARDED_BY(m_cs_fee_estimator);
    unsigned int historicalBest GUARDED_BY(m_cs_fee_estimator);
    struct TxStatsInfo
    {
        unsigned int blockHeight;
        unsigned int bucketIndex;
        TxStatsInfo() : blockHeight(0), bucketIndex(0) {}
    };
    std::map<uint256, TxStatsInfo> mapMemPoolTxs GUARDED_BY(m_cs_fee_estimator);
    std::unique_ptr<TxConfirmStats> feeStats PT_GUARDED_BY(m_cs_fee_estimator);
    std::unique_ptr<TxConfirmStats> shortStats PT_GUARDED_BY(m_cs_fee_estimator);
    std::unique_ptr<TxConfirmStats> longStats PT_GUARDED_BY(m_cs_fee_estimator);
    unsigned int trackedTxs GUARDED_BY(m_cs_fee_estimator);
    unsigned int untrackedTxs GUARDED_BY(m_cs_fee_estimator);
    std::vector<double> buckets GUARDED_BY(m_cs_fee_estimator);
    std::map<double, unsigned int> bucketMap GUARDED_BY(m_cs_fee_estimator);
    bool processBlockTx(unsigned int nBlockHeight, const CTxMemPoolEntry* entry) EXCLUSIVE_LOCKS_REQUIRED(m_cs_fee_estimator);
    double estimateCombinedFee(unsigned int confTarget, double successThreshold, bool checkShorterHorizon, EstimationResult *result) const EXCLUSIVE_LOCKS_REQUIRED(m_cs_fee_estimator);
    double estimateConservativeFee(unsigned int doubleTarget, EstimationResult *result) const EXCLUSIVE_LOCKS_REQUIRED(m_cs_fee_estimator);
    unsigned int BlockSpan() const EXCLUSIVE_LOCKS_REQUIRED(m_cs_fee_estimator);
    unsigned int HistoricalBlockSpan() const EXCLUSIVE_LOCKS_REQUIRED(m_cs_fee_estimator);
    unsigned int MaxUsableEstimate() const EXCLUSIVE_LOCKS_REQUIRED(m_cs_fee_estimator);
    bool _removeTx(const uint256& hash, bool inBlock)
        EXCLUSIVE_LOCKS_REQUIRED(m_cs_fee_estimator);
};
class FeeFilterRounder
{
private:
    static constexpr double MAX_FILTER_FEERATE = 1e7;
    static constexpr double FEE_FILTER_SPACING = 1.1;
public:
    explicit FeeFilterRounder(const CFeeRate& minIncrementalFee);
    CAmount round(CAmount currentMinFee);
private:
    std::set<double> feeset;
    FastRandomContext insecure_rand;
};
#endif
