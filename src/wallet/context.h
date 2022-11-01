#ifndef LCRYP_WALLET_CONTEXT_H
#define LCRYP_WALLET_CONTEXT_H
#include <sync.h>
#include <functional>
#include <list>
#include <memory>
#include <vector>
class ArgsManager;
namespace interfaces {
class Chain;
class Wallet;
}
namespace wallet {
class CWallet;
using LoadWalletFn = std::function<void(std::unique_ptr<interfaces::Wallet> wallet)>;
struct WalletContext {
    interfaces::Chain* chain{nullptr};
    ArgsManager* args{nullptr};
    Mutex wallets_mutex;
    std::vector<std::shared_ptr<CWallet>> wallets GUARDED_BY(wallets_mutex);
    std::list<LoadWalletFn> wallet_load_fns GUARDED_BY(wallets_mutex);
    WalletContext();
    ~WalletContext();
};
}
#endif
