#ifndef LCRYP_INTERFACES_INIT_H
#define LCRYP_INTERFACES_INIT_H
#include <memory>
namespace node {
struct NodeContext;
}
namespace interfaces {
class Chain;
class Echo;
class Ipc;
class Node;
class WalletLoader;
class Init
{
public:
    virtual ~Init() = default;
    virtual std::unique_ptr<Node> makeNode();
    virtual std::unique_ptr<Chain> makeChain();
    virtual std::unique_ptr<WalletLoader> makeWalletLoader(Chain& chain);
    virtual std::unique_ptr<Echo> makeEcho();
    virtual Ipc* ipc();
};
std::unique_ptr<Init> MakeNodeInit(node::NodeContext& node, int argc, char* argv[], int& exit_status);
std::unique_ptr<Init> MakeWalletInit(int argc, char* argv[], int& exit_status);
std::unique_ptr<Init> MakeGuiInit(int argc, char* argv[]);
}
#endif
