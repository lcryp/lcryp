#ifndef LCRYP_NODE_CONTEXT_H
#define LCRYP_NODE_CONTEXT_H
#include <kernel/context.h>
#include <cassert>
#include <functional>
#include <memory>
#include <vector>
class ArgsManager;
class BanMan;
class AddrMan;
class CBlockPolicyEstimator;
class CConnman;
class CScheduler;
class CTxMemPool;
class ChainstateManager;
class NetGroupManager;
class PeerManager;
namespace interfaces {
class Chain;
class ChainClient;
class Init;
class WalletLoader;
}
namespace node {
struct NodeContext {
    std::unique_ptr<kernel::Context> kernel;
    interfaces::Init* init{nullptr};
    std::unique_ptr<AddrMan> addrman;
    std::unique_ptr<CConnman> connman;
    std::unique_ptr<CTxMemPool> mempool;
    std::unique_ptr<const NetGroupManager> netgroupman;
    std::unique_ptr<CBlockPolicyEstimator> fee_estimator;
    std::unique_ptr<PeerManager> peerman;
    std::unique_ptr<ChainstateManager> chainman;
    std::unique_ptr<BanMan> banman;
    ArgsManager* args{nullptr};
    std::unique_ptr<interfaces::Chain> chain;
    std::vector<std::unique_ptr<interfaces::ChainClient>> chain_clients;
    interfaces::WalletLoader* wallet_loader{nullptr};
    std::unique_ptr<CScheduler> scheduler;
    std::function<void()> rpc_interruption_point = [] {};
    NodeContext();
    ~NodeContext();
};
}
#endif
