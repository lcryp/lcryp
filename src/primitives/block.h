#ifndef LCRYP_PRIMITIVES_BLOCK_H
#define LCRYP_PRIMITIVES_BLOCK_H
#include <primitives/transaction.h>
#include <serialize.h>
#include <uint256.h>
#include <util/time.h>
class CBlockHeader
{
public:
    int32_t nVersion;
    uint256 hashPrevBlock;
    uint256 hashMerkleRoot;
    uint32_t nTime;
    uint32_t nBits;
    uint32_t nNonce;
    CBlockHeader()
    {
        SetNull();
    }
    SERIALIZE_METHODS(CBlockHeader, obj) { READWRITE(obj.nVersion, obj.hashPrevBlock, obj.hashMerkleRoot, obj.nTime, obj.nBits, obj.nNonce); }
    void SetNull()
    {
        nVersion = 0;
        hashPrevBlock.SetNull();
        hashMerkleRoot.SetNull();
        nTime = 0;
        nBits = 0;
        nNonce = 0;
    }
    bool IsNull() const
    {
        return (nBits == 0);
    }
    uint256 GetHash() const;
    NodeSeconds Time() const
    {
        return NodeSeconds{std::chrono::seconds{nTime}};
    }
    int64_t GetBlockTime() const
    {
        return (int64_t)nTime;
    }
};
class CBlock : public CBlockHeader
{
public:
    std::vector<CTransactionRef> vtx;
    mutable bool fChecked;
    CBlock()
    {
        SetNull();
    }
    CBlock(const CBlockHeader &header)
    {
        SetNull();
        *(static_cast<CBlockHeader*>(this)) = header;
    }
    SERIALIZE_METHODS(CBlock, obj)
    {
        READWRITEAS(CBlockHeader, obj);
        READWRITE(obj.vtx);
    }
    void SetNull()
    {
        CBlockHeader::SetNull();
        vtx.clear();
        fChecked = false;
    }
    CBlockHeader GetBlockHeader() const
    {
        CBlockHeader block;
        block.nVersion       = nVersion;
        block.hashPrevBlock  = hashPrevBlock;
        block.hashMerkleRoot = hashMerkleRoot;
        block.nTime          = nTime;
        block.nBits          = nBits;
        block.nNonce         = nNonce;
        return block;
    }
    std::string ToString() const;
};
struct CBlockLocator
{
    std::vector<uint256> vHave;
    CBlockLocator() {}
    explicit CBlockLocator(std::vector<uint256>&& have) : vHave(std::move(have)) {}
    SERIALIZE_METHODS(CBlockLocator, obj)
    {
        int nVersion = s.GetVersion();
        if (!(s.GetType() & SER_GETHASH))
            READWRITE(nVersion);
        READWRITE(obj.vHave);
    }
    void SetNull()
    {
        vHave.clear();
    }
    bool IsNull() const
    {
        return vHave.empty();
    }
};
#endif
