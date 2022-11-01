#ifndef LCRYP_MERKLEBLOCK_H
#define LCRYP_MERKLEBLOCK_H
#include <common/bloom.h>
#include <primitives/block.h>
#include <serialize.h>
#include <uint256.h>
#include <vector>
std::vector<unsigned char> BitsToBytes(const std::vector<bool>& bits);
std::vector<bool> BytesToBits(const std::vector<unsigned char>& bytes);
class CPartialMerkleTree
{
protected:
    unsigned int nTransactions;
    std::vector<bool> vBits;
    std::vector<uint256> vHash;
    bool fBad;
    unsigned int CalcTreeWidth(int height) const {
        return (nTransactions+(1 << height)-1) >> height;
    }
    uint256 CalcHash(int height, unsigned int pos, const std::vector<uint256> &vTxid);
    void TraverseAndBuild(int height, unsigned int pos, const std::vector<uint256> &vTxid, const std::vector<bool> &vMatch);
    uint256 TraverseAndExtract(int height, unsigned int pos, unsigned int &nBitsUsed, unsigned int &nHashUsed, std::vector<uint256> &vMatch, std::vector<unsigned int> &vnIndex);
public:
    SERIALIZE_METHODS(CPartialMerkleTree, obj)
    {
        READWRITE(obj.nTransactions, obj.vHash);
        std::vector<unsigned char> bytes;
        SER_WRITE(obj, bytes = BitsToBytes(obj.vBits));
        READWRITE(bytes);
        SER_READ(obj, obj.vBits = BytesToBits(bytes));
        SER_READ(obj, obj.fBad = false);
    }
    CPartialMerkleTree(const std::vector<uint256> &vTxid, const std::vector<bool> &vMatch);
    CPartialMerkleTree();
    uint256 ExtractMatches(std::vector<uint256> &vMatch, std::vector<unsigned int> &vnIndex);
    unsigned int GetNumTransactions() const { return nTransactions; };
};
class CMerkleBlock
{
public:
    CBlockHeader header;
    CPartialMerkleTree txn;
    std::vector<std::pair<unsigned int, uint256> > vMatchedTxn;
    CMerkleBlock(const CBlock& block, CBloomFilter& filter) : CMerkleBlock(block, &filter, nullptr) { }
    CMerkleBlock(const CBlock& block, const std::set<uint256>& txids) : CMerkleBlock(block, nullptr, &txids) { }
    CMerkleBlock() {}
    SERIALIZE_METHODS(CMerkleBlock, obj) { READWRITE(obj.header, obj.txn); }
private:
    CMerkleBlock(const CBlock& block, CBloomFilter* filter, const std::set<uint256>* txids);
};
#endif
