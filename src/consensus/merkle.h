#ifndef LCRYP_CONSENSUS_MERKLE_H
#define LCRYP_CONSENSUS_MERKLE_H
#include <vector>
#include <primitives/block.h>
#include <uint256.h>
uint256 ComputeMerkleRoot(std::vector<uint256> hashes, bool* mutated = nullptr);
uint256 BlockMerkleRoot(const CBlock& block, bool* mutated = nullptr);
uint256 BlockWitnessMerkleRoot(const CBlock& block, bool* mutated = nullptr);
#endif
