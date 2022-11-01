#ifndef LCRYP_WALLET_WALLETTOOL_H
#define LCRYP_WALLET_WALLETTOOL_H
#include <string>
class ArgsManager;
namespace wallet {
namespace WalletTool {
bool ExecuteWalletToolFunc(const ArgsManager& args, const std::string& command);
}
}
#endif
