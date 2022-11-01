#ifndef LCRYP_WALLET_RPC_UTIL_H
#define LCRYP_WALLET_RPC_UTIL_H
#include <script/script.h>
#include <any>
#include <memory>
#include <string>
#include <vector>
class JSONRPCRequest;
class UniValue;
struct bilingual_str;
namespace wallet {
class CWallet;
class LegacyScriptPubKeyMan;
enum class DatabaseStatus;
struct WalletContext;
extern const std::string HELP_REQUIRING_PASSPHRASE;
std::shared_ptr<CWallet> GetWalletForJSONRPCRequest(const JSONRPCRequest& request);
bool GetWalletNameFromJSONRPCRequest(const JSONRPCRequest& request, std::string& wallet_name);
void EnsureWalletIsUnlocked(const CWallet&);
WalletContext& EnsureWalletContext(const std::any& context);
LegacyScriptPubKeyMan& EnsureLegacyScriptPubKeyMan(CWallet& wallet, bool also_create = false);
const LegacyScriptPubKeyMan& EnsureConstLegacyScriptPubKeyMan(const CWallet& wallet);
bool GetAvoidReuseFlag(const CWallet& wallet, const UniValue& param);
bool ParseIncludeWatchonly(const UniValue& include_watchonly, const CWallet& wallet);
std::string LabelFromValue(const UniValue& value);
void PushParentDescriptors(const CWallet& wallet, const CScript& script_pubkey, UniValue& entry);
void HandleWalletError(const std::shared_ptr<CWallet> wallet, DatabaseStatus& status, bilingual_str& error);
}
#endif
