#include <pow.h>
#include <arith_uint256.h>
#include <chain.h>
#include <primitives/block.h>
#include <uint256.h>
#include <chainparams.h>
#include <logging.h>
#include <stdlib.h>

//
// MIDAS
//

bool avgRecentTimestamps(const CBlockIndex* pindexLast, int64_t* avgOf5, int64_t* avgOf7, int64_t* avgOf9, int64_t* avgOf17, int64_t* BlockHeightTime, const Consensus::Params& params)
{
    try {
        int blockoffset = 0;
        int count_limit = 0;
        int64_t newblocktime;
        int64_t blocktime = 0;
        *avgOf5 = 0;
        *avgOf7 = 0;
        *avgOf9 = 0;
        *avgOf17 = 0;
        if (pindexLast != nullptr)
        {
            blocktime = pindexLast->GetBlockTime();
            for (blockoffset = 0; blockoffset < 17; blockoffset++)
            {
                newblocktime = blocktime;
                pindexLast = pindexLast->pprev;
                if (pindexLast != nullptr)
                {
                    blocktime = pindexLast->GetBlockTime();
                    count_limit++;
                }
                else break;
                if (blockoffset < 5) *avgOf5 += (newblocktime - blocktime);
                if (blockoffset < 7) *avgOf7 += (newblocktime - blocktime);
                if (blockoffset < 9) *avgOf9 += (newblocktime - blocktime);
                if (blockoffset < 17) *avgOf17 += (newblocktime - blocktime);
            }
        }
        if (count_limit != 0)
        {
            *avgOf5 /= std::min<int64_t>(count_limit, 5);
            *avgOf7 /= std::min<int64_t>(count_limit, 7);
            *avgOf9 /= std::min<int64_t>(count_limit, 9);
            *avgOf17 /= std::min<int64_t>(count_limit, 17);
        }
        else
        {
            *avgOf5 = params.nPowTargetSpacing;
            *avgOf7 = params.nPowTargetSpacing;
            *avgOf9 = params.nPowTargetSpacing;
            *avgOf17 = params.nPowTargetSpacing;
        }
        if (blocktime==0)
            *BlockHeightTime = Params().GenesisBlock().GetBlockTime();
        else
            *BlockHeightTime = blocktime + count_limit * params.nPowTargetSpacing;
        return true;
    }
    catch (const std::runtime_error& e)
    {
        LogPrintf("Runtime mining error: %s [%s:%s:%u]\n", e.what(), __FILE__, __func__, __LINE__);
        return false;
    }
}
unsigned int GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlockHeader* pblock, const Consensus::Params& params)
{
    try {
        assert(pindexLast != nullptr);
        int64_t avgOf5;
        int64_t avgOf9;
        int64_t avgOf7;
        int64_t avgOf17;
        int64_t toofast;
        int64_t tooslow;
        int64_t difficultyfactor = 10000;
        int64_t now;
        int64_t reg;
        int64_t BlockHeightTime;
        int64_t nFastInterval = (params.nPowTargetSpacing * 9) / 10;
        int64_t nSlowInterval = (params.nPowTargetSpacing * 11) / 10;
        int64_t nIntervalDesired = params.nPowTargetSpacing;
        if (pindexLast == NULL)
            return UintToArith256(params.powLimit).GetCompact();
        if (!avgRecentTimestamps(pindexLast, &avgOf5, &avgOf7, &avgOf9, &avgOf17, &BlockHeightTime, params))
			return UintToArith256(params.powLimit).GetCompact();
        now = pindexLast->GetBlockTime();
        if (now < BlockHeightTime + params.DifficultyAdjustmentInterval() && now > BlockHeightTime)
        {
            nIntervalDesired = ((params.DifficultyAdjustmentInterval() - (now - BlockHeightTime)) * params.nPowTargetSpacing +  (now - BlockHeightTime) * nFastInterval) / params.DifficultyAdjustmentInterval();
        }
        else if (now + params.DifficultyAdjustmentInterval() > BlockHeightTime && now < BlockHeightTime)
        {
            nIntervalDesired = ((params.DifficultyAdjustmentInterval() - (BlockHeightTime - now)) * params.nPowTargetSpacing + (BlockHeightTime - now) * nSlowInterval) / params.DifficultyAdjustmentInterval();
        }
        else if (now < BlockHeightTime)
        {
            nIntervalDesired = nSlowInterval;
        }
        else if (now > BlockHeightTime)
        {
            nIntervalDesired = nFastInterval;
        }
        else
        {
            nIntervalDesired = params.nPowTargetSpacing;
        }
        toofast = (nIntervalDesired * 2) / 3;
        tooslow = (nIntervalDesired * 3) / 2;
        if (avgOf5 < toofast && avgOf7 < toofast && avgOf9 < toofast && avgOf17 < nIntervalDesired)
        {
            LogPrintf("MIDAS EMERGENCY DIFFICULTY INCREASE\n");
            reg = 1;
            if ((avgOf5 > avgOf7) && (avgOf5 > avgOf9))
            {
                reg = avgOf5;
            }
            else  if ((avgOf7 > avgOf9))
            {
                reg = avgOf7;
            }
            else
            {
                reg = avgOf9;
            }
            difficultyfactor *= 4 * toofast;
            difficultyfactor /= (reg + 3 * toofast);
        }
        else if (avgOf5 > tooslow && avgOf7 > tooslow && avgOf9 > tooslow && avgOf17 > nIntervalDesired)
        {
            LogPrintf("MIDAS EMERGENCY DIFFICULTY REDUCTION\n");
            reg = 1;
            if ((avgOf5 < avgOf7) && (avgOf5 < avgOf9))
            {
                reg = avgOf5;
            }
            else if ((avgOf7 < avgOf9))
            {
                reg = avgOf7;
            }
            else
            {
                reg = avgOf9;
            }
            difficultyfactor *= 4 * tooslow;
            difficultyfactor /= reg + 3 * tooslow;
        }
        else if (avgOf5 < nIntervalDesired && avgOf7 < nIntervalDesired && avgOf9 < nIntervalDesired && avgOf17 < nIntervalDesired)
        {
            LogPrintf("MIDAS DIFFICULTY INCREASE\n");
            reg = 1;
            if ((avgOf5 > avgOf7) && (avgOf5 > avgOf9) && (avgOf5 > avgOf17))
            {
                reg = avgOf5;
            }
            else  if ((avgOf7 > avgOf9) && (avgOf7 > avgOf17))
            {
                reg = avgOf7;
            }
            else if (avgOf9 > avgOf17)
            {
                reg = avgOf9;
            }
            else
            {
                reg = avgOf17;
            }
            difficultyfactor *= (4 * nIntervalDesired);
            difficultyfactor /= (reg + 3 * nIntervalDesired);
        }
        else if (avgOf5 > nIntervalDesired && avgOf7 > nIntervalDesired && avgOf9 > nIntervalDesired && avgOf17 > nIntervalDesired)
        {
            LogPrintf("MIDAS DIFFICULTY REDUCTION\n");
            reg = 0;
            if ((avgOf5 < avgOf7) && (avgOf5 < avgOf9) && (avgOf5 < avgOf17))
            {
                reg = avgOf5;
            }
            else if ((avgOf7 < avgOf9) && (avgOf7 < avgOf17))
            {
                reg = avgOf7;
            }
            else if (avgOf9 < avgOf17)
            {
                reg = avgOf9;
            }
            else
            {
                reg = avgOf17;
            }
            difficultyfactor *= 4 * nIntervalDesired;
            difficultyfactor /= (reg + 3 * nIntervalDesired);
        }
        arith_uint256 bnNew;
        arith_uint256 bnOld;
        bnOld.SetCompact(pindexLast->nBits);
        if (difficultyfactor != 10000)
        {
            bnNew = bnOld / difficultyfactor;
            bnNew *= 10000;
        }
        else
        {
            bnNew = bnOld;
        }
        if (bnNew > UintToArith256(params.powLimit))
            bnNew = UintToArith256(params.powLimit);
        LogPrintf("Actual time %d, Scheduled time for this block height = %d\n", now, BlockHeightTime);
        LogPrintf("Nominal block interval = %d, regulating on interval %d to get back to schedule.\n", params.nPowTargetSpacing, nIntervalDesired);
        LogPrintf("Intervals of last 5/7/9/17 blocks = %d / %d / %d / %d / %d.\n", params.nPowTargetSpacing, avgOf5, avgOf7, avgOf9, avgOf17);
        LogPrintf("Difficulty Before Adjustment: %08x  %s\n", pindexLast->nBits, bnOld.ToString());
        LogPrintf("Difficulty After Adjustment:  %08x  %s\n", bnNew.GetCompact(), bnNew.ToString());
        return bnNew.GetCompact();
    }
    catch (const std::runtime_error& e)
    {
        LogPrintf("Runtime mining error: %s [%s:%s:%u]\n", e.what(), __FILE__, __func__, __LINE__);
        return UintToArith256(params.powLimit).GetCompact();
    }
}
unsigned int CalculateNextWorkRequired(const CBlockIndex* pindexLast, int64_t nFirstBlockTime, const Consensus::Params& params)
{
    if (params.fPowNoRetargeting)
        return pindexLast->nBits;
    int64_t nActualTimespan = pindexLast->GetBlockTime() - nFirstBlockTime;
    if (nActualTimespan < params.nPowTargetTimespan/4)
        nActualTimespan = params.nPowTargetTimespan/4;
    if (nActualTimespan > params.nPowTargetTimespan*4)
        nActualTimespan = params.nPowTargetTimespan*4;
    const arith_uint256 bnPowLimit = UintToArith256(params.powLimit);
    arith_uint256 bnNew;
    bnNew.SetCompact(pindexLast->nBits);
    bnNew *= nActualTimespan;
    bnNew /= params.nPowTargetTimespan;
    if (bnNew > bnPowLimit)
        bnNew = bnPowLimit;
    return bnNew.GetCompact();
}
bool PermittedDifficultyTransition(const Consensus::Params& params, int64_t height, uint32_t old_nbits, uint32_t new_nbits)
{
    if (params.fPowAllowMinDifficultyBlocks) return true;
    if (height % params.DifficultyAdjustmentInterval() == 0) {
        int64_t smallest_timespan = params.nPowTargetTimespan/4;
        int64_t largest_timespan = params.nPowTargetTimespan*4;
        const arith_uint256 pow_limit = UintToArith256(params.powLimit);
        arith_uint256 observed_new_target;
        observed_new_target.SetCompact(new_nbits);
        arith_uint256 largest_difficulty_target;
        largest_difficulty_target.SetCompact(old_nbits);
        largest_difficulty_target *= largest_timespan;
        largest_difficulty_target /= params.nPowTargetTimespan;
        if (largest_difficulty_target > pow_limit) {
            largest_difficulty_target = pow_limit;
        }
        arith_uint256 maximum_new_target;
        maximum_new_target.SetCompact(largest_difficulty_target.GetCompact());
        if (maximum_new_target < observed_new_target) return false;
        arith_uint256 smallest_difficulty_target;
        smallest_difficulty_target.SetCompact(old_nbits);
        smallest_difficulty_target *= smallest_timespan;
        smallest_difficulty_target /= params.nPowTargetTimespan;
        if (smallest_difficulty_target > pow_limit) {
            smallest_difficulty_target = pow_limit;
        }
        arith_uint256 minimum_new_target;
        minimum_new_target.SetCompact(smallest_difficulty_target.GetCompact());
        if (minimum_new_target > observed_new_target) return false;
    } else if (old_nbits != new_nbits) {
        return false;
    }
    return true;
}
bool CheckProofOfWork(uint256 hash, unsigned int nBits, const Consensus::Params& params)
{
    bool fNegative;
    bool fOverflow;
    arith_uint256 bnTarget;
    bnTarget.SetCompact(nBits, &fNegative, &fOverflow);
    if (fNegative || bnTarget == 0 || fOverflow || bnTarget > UintToArith256(params.powLimit))
        return false;
    if (UintToArith256(hash) > bnTarget)
        return false;
    return true;
}
