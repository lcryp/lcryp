#ifndef LCRYP_POW_H
#define LCRYP_POW_H
#include <consensus/params.h>
#include <stdint.h>
class CBlockHeader;
class CBlockIndex;
class uint256;
unsigned int GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params&);
unsigned int CalculateNextWorkRequired(const CBlockIndex* pindexLast, int64_t nFirstBlockTime, const Consensus::Params&);
bool CheckProofOfWork(uint256 hash, unsigned int nBits, const Consensus::Params&);
bool PermittedDifficultyTransition(const Consensus::Params& params, int64_t height, uint32_t old_nbits, uint32_t new_nbits);
#endif
