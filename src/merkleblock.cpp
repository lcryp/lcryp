#include <merkleblock.h>
#include <hash.h>
#include <consensus/consensus.h>
std::vector<unsigned char> BitsToBytes(const std::vector<bool>& bits)
{
    std::vector<unsigned char> ret((bits.size() + 7) / 8);
    for (unsigned int p = 0; p < bits.size(); p++) {
        ret[p / 8] |= bits[p] << (p % 8);
    }
    return ret;
}
std::vector<bool> BytesToBits(const std::vector<unsigned char>& bytes)
{
    std::vector<bool> ret(bytes.size() * 8);
    for (unsigned int p = 0; p < ret.size(); p++) {
        ret[p] = (bytes[p / 8] & (1 << (p % 8))) != 0;
    }
    return ret;
}
CMerkleBlock::CMerkleBlock(const CBlock& block, CBloomFilter* filter, const std::set<uint256>* txids)
{
    header = block.GetBlockHeader();
    std::vector<bool> vMatch;
    std::vector<uint256> vHashes;
    vMatch.reserve(block.vtx.size());
    vHashes.reserve(block.vtx.size());
    for (unsigned int i = 0; i < block.vtx.size(); i++)
    {
        const uint256& hash = block.vtx[i]->GetHash();
        if (txids && txids->count(hash)) {
            vMatch.push_back(true);
        } else if (filter && filter->IsRelevantAndUpdate(*block.vtx[i])) {
            vMatch.push_back(true);
            vMatchedTxn.emplace_back(i, hash);
        } else {
            vMatch.push_back(false);
        }
        vHashes.push_back(hash);
    }
    txn = CPartialMerkleTree(vHashes, vMatch);
}
uint256 CPartialMerkleTree::CalcHash(int height, unsigned int pos, const std::vector<uint256> &vTxid) {
    assert(vTxid.size() != 0);
    if (height == 0) {
        return vTxid[pos];
    } else {
        uint256 left = CalcHash(height-1, pos*2, vTxid), right;
        if (pos*2+1 < CalcTreeWidth(height-1))
            right = CalcHash(height-1, pos*2+1, vTxid);
        else
            right = left;
        return Hash(left, right);
    }
}
void CPartialMerkleTree::TraverseAndBuild(int height, unsigned int pos, const std::vector<uint256> &vTxid, const std::vector<bool> &vMatch) {
    bool fParentOfMatch = false;
    for (unsigned int p = pos << height; p < (pos+1) << height && p < nTransactions; p++)
        fParentOfMatch |= vMatch[p];
    vBits.push_back(fParentOfMatch);
    if (height==0 || !fParentOfMatch) {
        vHash.push_back(CalcHash(height, pos, vTxid));
    } else {
        TraverseAndBuild(height-1, pos*2, vTxid, vMatch);
        if (pos*2+1 < CalcTreeWidth(height-1))
            TraverseAndBuild(height-1, pos*2+1, vTxid, vMatch);
    }
}
uint256 CPartialMerkleTree::TraverseAndExtract(int height, unsigned int pos, unsigned int &nBitsUsed, unsigned int &nHashUsed, std::vector<uint256> &vMatch, std::vector<unsigned int> &vnIndex) {
    if (nBitsUsed >= vBits.size()) {
        fBad = true;
        return uint256();
    }
    bool fParentOfMatch = vBits[nBitsUsed++];
    if (height==0 || !fParentOfMatch) {
        if (nHashUsed >= vHash.size()) {
            fBad = true;
            return uint256();
        }
        const uint256 &hash = vHash[nHashUsed++];
        if (height==0 && fParentOfMatch) {
            vMatch.push_back(hash);
            vnIndex.push_back(pos);
        }
        return hash;
    } else {
        uint256 left = TraverseAndExtract(height-1, pos*2, nBitsUsed, nHashUsed, vMatch, vnIndex), right;
        if (pos*2+1 < CalcTreeWidth(height-1)) {
            right = TraverseAndExtract(height-1, pos*2+1, nBitsUsed, nHashUsed, vMatch, vnIndex);
            if (right == left) {
                fBad = true;
            }
        } else {
            right = left;
        }
        return Hash(left, right);
    }
}
CPartialMerkleTree::CPartialMerkleTree(const std::vector<uint256> &vTxid, const std::vector<bool> &vMatch) : nTransactions(vTxid.size()), fBad(false) {
    vBits.clear();
    vHash.clear();
    int nHeight = 0;
    while (CalcTreeWidth(nHeight) > 1)
        nHeight++;
    TraverseAndBuild(nHeight, 0, vTxid, vMatch);
}
CPartialMerkleTree::CPartialMerkleTree() : nTransactions(0), fBad(true) {}
uint256 CPartialMerkleTree::ExtractMatches(std::vector<uint256> &vMatch, std::vector<unsigned int> &vnIndex) {
    vMatch.clear();
    if (nTransactions == 0)
        return uint256();
    if (nTransactions > MAX_BLOCK_WEIGHT / MIN_TRANSACTION_WEIGHT)
        return uint256();
    if (vHash.size() > nTransactions)
        return uint256();
    if (vBits.size() < vHash.size())
        return uint256();
    int nHeight = 0;
    while (CalcTreeWidth(nHeight) > 1)
        nHeight++;
    unsigned int nBitsUsed = 0, nHashUsed = 0;
    uint256 hashMerkleRoot = TraverseAndExtract(nHeight, 0, nBitsUsed, nHashUsed, vMatch, vnIndex);
    if (fBad)
        return uint256();
    if ((nBitsUsed+7)/8 != (vBits.size()+7)/8)
        return uint256();
    if (nHashUsed != vHash.size())
        return uint256();
    return hashMerkleRoot;
}
