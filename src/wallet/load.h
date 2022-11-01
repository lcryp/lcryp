#ifndef LCRYP_WALLET_LOAD_H
#define LCRYP_WALLET_LOAD_H
#include <string>
#include <vector>
class ArgsManager;
class CScheduler;
namespace interfaces {
class Chain;
}
namespace wallet {
struct WalletContext;
bool VerifyWallets(WalletContext& context);
bool LoadWallets(WalletContext& context);
void StartWallets(WalletContext& context, CScheduler& scheduler);
void FlushWallets(WalletContext& context);
void StopWallets(WalletContext& context);
void UnloadWallets(WalletContext& context);
}
#endif
