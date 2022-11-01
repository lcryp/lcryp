#include <interfaces/chain.h>
#include <interfaces/echo.h>
#include <interfaces/init.h>
#include <interfaces/node.h>
#include <interfaces/wallet.h>
namespace interfaces {
std::unique_ptr<Node> Init::makeNode() { return {}; }
std::unique_ptr<Chain> Init::makeChain() { return {}; }
std::unique_ptr<WalletLoader> Init::makeWalletLoader(Chain& chain) { return {}; }
std::unique_ptr<Echo> Init::makeEcho() { return {}; }
Ipc* Init::ipc() { return nullptr; }
}
