#ifndef LCRYP_KERNEL_CHAIN_H
#define LCRYP_KERNEL_CHAIN_H
class CBlock;
class CBlockIndex;
namespace interfaces {
struct BlockInfo;
}
namespace kernel {
interfaces::BlockInfo MakeBlockInfo(const CBlockIndex* block_index, const CBlock* data = nullptr);
}
#endif
