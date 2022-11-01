#ifndef LCRYP_CHAIN_H
#define LCRYP_CHAIN_H
#include <arith_uint256.h>
#include <consensus/params.h>
#include <flatfile.h>
#include <primitives/block.h>
#include <sync.h>
#include <uint256.h>
#include <util/time.h>
#include <vector>
static constexpr int64_t MAX_FUTURE_BLOCK_TIME = 2 * 60 * 60;
static constexpr int64_t TIMESTAMP_WINDOW = MAX_FUTURE_BLOCK_TIME;
static constexpr int64_t MAX_BLOCK_TIME_GAP = 1 * 90 * 60;
extern RecursiveMutex cs_main;
class CBlockFileInfo
{
public:
    unsigned int nBlocks;
    unsigned int nSize;
    unsigned int nUndoSize;
    unsigned int nHeightFirst;
    unsigned int nHeightLast;
    uint64_t nTimeFirst;
    uint64_t nTimeLast;
    SERIALIZE_METHODS(CBlockFileInfo, obj)
    {
        READWRITE(VARINT(obj.nBlocks));
        READWRITE(VARINT(obj.nSize));
        READWRITE(VARINT(obj.nUndoSize));
        READWRITE(VARINT(obj.nHeightFirst));
        READWRITE(VARINT(obj.nHeightLast));
        READWRITE(VARINT(obj.nTimeFirst));
        READWRITE(VARINT(obj.nTimeLast));
    }
    void SetNull()
    {
        nBlocks = 0;
        nSize = 0;
        nUndoSize = 0;
        nHeightFirst = 0;
        nHeightLast = 0;
        nTimeFirst = 0;
        nTimeLast = 0;
    }
    CBlockFileInfo()
    {
        SetNull();
    }
    std::string ToString() const;
    void AddBlock(unsigned int nHeightIn, uint64_t nTimeIn)
    {
        if (nBlocks == 0 || nHeightFirst > nHeightIn)
            nHeightFirst = nHeightIn;
        if (nBlocks == 0 || nTimeFirst > nTimeIn)
            nTimeFirst = nTimeIn;
        nBlocks++;
        if (nHeightIn > nHeightLast)
            nHeightLast = nHeightIn;
        if (nTimeIn > nTimeLast)
            nTimeLast = nTimeIn;
    }
};
enum BlockStatus : uint32_t {
    BLOCK_VALID_UNKNOWN      =    0,
    BLOCK_VALID_RESERVED     =    1,
    BLOCK_VALID_TREE         =    2,
    BLOCK_VALID_TRANSACTIONS =    3,
    BLOCK_VALID_CHAIN        =    4,
    BLOCK_VALID_SCRIPTS      =    5,
    BLOCK_VALID_MASK         =   BLOCK_VALID_RESERVED | BLOCK_VALID_TREE | BLOCK_VALID_TRANSACTIONS |
                                 BLOCK_VALID_CHAIN | BLOCK_VALID_SCRIPTS,
    BLOCK_HAVE_DATA          =    8,
    BLOCK_HAVE_UNDO          =   16,
    BLOCK_HAVE_MASK          =   BLOCK_HAVE_DATA | BLOCK_HAVE_UNDO,
    BLOCK_FAILED_VALID       =   32,
    BLOCK_FAILED_CHILD       =   64,
    BLOCK_FAILED_MASK        =   BLOCK_FAILED_VALID | BLOCK_FAILED_CHILD,
    BLOCK_OPT_WITNESS        =   128,
    BLOCK_ASSUMED_VALID      =   256,
};
class CBlockIndex
{
public:
    const uint256* phashBlock{nullptr};
    CBlockIndex* pprev{nullptr};
    CBlockIndex* pskip{nullptr};
    int nHeight{0};
    int nFile GUARDED_BY(::cs_main){0};
    unsigned int nDataPos GUARDED_BY(::cs_main){0};
    unsigned int nUndoPos GUARDED_BY(::cs_main){0};
    arith_uint256 nChainWork{};
    unsigned int nTx{0};
    unsigned int nChainTx{0};
    uint32_t nStatus GUARDED_BY(::cs_main){0};
    int32_t nVersion{0};
    uint256 hashMerkleRoot{};
    uint32_t nTime{0};
    uint32_t nBits{0};
    uint32_t nNonce{0};
    int32_t nSequenceId{0};
    unsigned int nTimeMax{0};
    CBlockIndex()
    {
    }
    explicit CBlockIndex(const CBlockHeader& block)
        : nVersion{block.nVersion},
          hashMerkleRoot{block.hashMerkleRoot},
          nTime{block.nTime},
          nBits{block.nBits},
          nNonce{block.nNonce}
    {
    }
    FlatFilePos GetBlockPos() const EXCLUSIVE_LOCKS_REQUIRED(::cs_main)
    {
        AssertLockHeld(::cs_main);
        FlatFilePos ret;
        if (nStatus & BLOCK_HAVE_DATA) {
            ret.nFile = nFile;
            ret.nPos = nDataPos;
        }
        return ret;
    }
    FlatFilePos GetUndoPos() const EXCLUSIVE_LOCKS_REQUIRED(::cs_main)
    {
        AssertLockHeld(::cs_main);
        FlatFilePos ret;
        if (nStatus & BLOCK_HAVE_UNDO) {
            ret.nFile = nFile;
            ret.nPos = nUndoPos;
        }
        return ret;
    }
    CBlockHeader GetBlockHeader() const
    {
        CBlockHeader block;
        block.nVersion = nVersion;
        if (pprev)
            block.hashPrevBlock = pprev->GetBlockHash();
        block.hashMerkleRoot = hashMerkleRoot;
        block.nTime = nTime;
        block.nBits = nBits;
        block.nNonce = nNonce;
        return block;
    }
    uint256 GetBlockHash() const
    {
        assert(phashBlock != nullptr);
        return *phashBlock;
    }
    bool HaveTxsDownloaded() const { return nChainTx != 0; }
    NodeSeconds Time() const
    {
        return NodeSeconds{std::chrono::seconds{nTime}};
    }
    int64_t GetBlockTime() const
    {
        return (int64_t)nTime;
    }
    int64_t GetBlockTimeMax() const
    {
        return (int64_t)nTimeMax;
    }
    static constexpr int nMedianTimeSpan = 11;
    int64_t GetMedianTimePast() const
    {
        int64_t pmedian[nMedianTimeSpan];
        int64_t* pbegin = &pmedian[nMedianTimeSpan];
        int64_t* pend = &pmedian[nMedianTimeSpan];
        const CBlockIndex* pindex = this;
        for (int i = 0; i < nMedianTimeSpan && pindex; i++, pindex = pindex->pprev)
            *(--pbegin) = pindex->GetBlockTime();
        std::sort(pbegin, pend);
        return pbegin[(pend - pbegin) / 2];
    }
    std::string ToString() const;
    bool IsValid(enum BlockStatus nUpTo = BLOCK_VALID_TRANSACTIONS) const
        EXCLUSIVE_LOCKS_REQUIRED(::cs_main)
    {
        AssertLockHeld(::cs_main);
        assert(!(nUpTo & ~BLOCK_VALID_MASK));
        if (nStatus & BLOCK_FAILED_MASK)
            return false;
        return ((nStatus & BLOCK_VALID_MASK) >= nUpTo);
    }
    bool IsAssumedValid() const EXCLUSIVE_LOCKS_REQUIRED(::cs_main)
    {
        AssertLockHeld(::cs_main);
        return nStatus & BLOCK_ASSUMED_VALID;
    }
    bool RaiseValidity(enum BlockStatus nUpTo) EXCLUSIVE_LOCKS_REQUIRED(::cs_main)
    {
        AssertLockHeld(::cs_main);
        assert(!(nUpTo & ~BLOCK_VALID_MASK));
        if (nStatus & BLOCK_FAILED_MASK) return false;
        if ((nStatus & BLOCK_VALID_MASK) < nUpTo) {
            if (nStatus & BLOCK_ASSUMED_VALID && nUpTo >= BLOCK_VALID_SCRIPTS) {
                nStatus &= ~BLOCK_ASSUMED_VALID;
            }
            nStatus = (nStatus & ~BLOCK_VALID_MASK) | nUpTo;
            return true;
        }
        return false;
    }
    void BuildSkip();
    CBlockIndex* GetAncestor(int height);
    const CBlockIndex* GetAncestor(int height) const;
};
arith_uint256 GetBlockProof(const CBlockIndex& block);
int64_t GetBlockProofEquivalentTime(const CBlockIndex& to, const CBlockIndex& from, const CBlockIndex& tip, const Consensus::Params&);
const CBlockIndex* LastCommonAncestor(const CBlockIndex* pa, const CBlockIndex* pb);
class CDiskBlockIndex : public CBlockIndex
{
public:
    uint256 hashPrev;
    CDiskBlockIndex()
    {
        hashPrev = uint256();
    }
    explicit CDiskBlockIndex(const CBlockIndex* pindex) : CBlockIndex(*pindex)
    {
        hashPrev = (pprev ? pprev->GetBlockHash() : uint256());
    }
    SERIALIZE_METHODS(CDiskBlockIndex, obj)
    {
        LOCK(::cs_main);
        int _nVersion = s.GetVersion();
        if (!(s.GetType() & SER_GETHASH)) READWRITE(VARINT_MODE(_nVersion, VarIntMode::NONNEGATIVE_SIGNED));
        READWRITE(VARINT_MODE(obj.nHeight, VarIntMode::NONNEGATIVE_SIGNED));
        READWRITE(VARINT(obj.nStatus));
        READWRITE(VARINT(obj.nTx));
        if (obj.nStatus & (BLOCK_HAVE_DATA | BLOCK_HAVE_UNDO)) READWRITE(VARINT_MODE(obj.nFile, VarIntMode::NONNEGATIVE_SIGNED));
        if (obj.nStatus & BLOCK_HAVE_DATA) READWRITE(VARINT(obj.nDataPos));
        if (obj.nStatus & BLOCK_HAVE_UNDO) READWRITE(VARINT(obj.nUndoPos));
        READWRITE(obj.nVersion);
        READWRITE(obj.hashPrev);
        READWRITE(obj.hashMerkleRoot);
        READWRITE(obj.nTime);
        READWRITE(obj.nBits);
        READWRITE(obj.nNonce);
    }
    uint256 ConstructBlockHash() const
    {
        CBlockHeader block;
        block.nVersion = nVersion;
        block.hashPrevBlock = hashPrev;
        block.hashMerkleRoot = hashMerkleRoot;
        block.nTime = nTime;
        block.nBits = nBits;
        block.nNonce = nNonce;
        return block.GetHash();
    }
    uint256 GetBlockHash() = delete;
    std::string ToString() = delete;
};
class CChain
{
private:
    std::vector<CBlockIndex*> vChain;
public:
    CChain() = default;
    CChain(const CChain&) = delete;
    CChain& operator=(const CChain&) = delete;
    CBlockIndex* Genesis() const
    {
        return vChain.size() > 0 ? vChain[0] : nullptr;
    }
    CBlockIndex* Tip() const
    {
        return vChain.size() > 0 ? vChain[vChain.size() - 1] : nullptr;
    }
    CBlockIndex* operator[](int nHeight) const
    {
        if (nHeight < 0 || nHeight >= (int)vChain.size())
            return nullptr;
        return vChain[nHeight];
    }
    bool Contains(const CBlockIndex* pindex) const
    {
        return (*this)[pindex->nHeight] == pindex;
    }
    CBlockIndex* Next(const CBlockIndex* pindex) const
    {
        if (Contains(pindex))
            return (*this)[pindex->nHeight + 1];
        else
            return nullptr;
    }
    int Height() const
    {
        return int(vChain.size()) - 1;
    }
    void SetTip(CBlockIndex& block);
    CBlockLocator GetLocator() const;
    const CBlockIndex* FindFork(const CBlockIndex* pindex) const;
    CBlockIndex* FindEarliestAtLeast(int64_t nTime, int height) const;
};
CBlockLocator GetLocator(const CBlockIndex* index);
std::vector<uint256> LocatorEntries(const CBlockIndex* index);
#endif
