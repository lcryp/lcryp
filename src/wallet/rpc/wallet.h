#ifndef LCRYP_WALLET_RPC_WALLET_H
#define LCRYP_WALLET_RPC_WALLET_H
#include <span.h>
class CRPCCommand;
namespace wallet {
Span<const CRPCCommand> GetWalletRPCCommands();
}
#endif
