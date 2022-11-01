#ifndef LCRYP_WALLET_DUMP_H
#define LCRYP_WALLET_DUMP_H
#include <fs.h>
#include <string>
#include <vector>
struct bilingual_str;
class ArgsManager;
namespace wallet {
class CWallet;
bool DumpWallet(const ArgsManager& args, CWallet& wallet, bilingual_str& error);
bool CreateFromDump(const ArgsManager& args, const std::string& name, const fs::path& wallet_path, bilingual_str& error, std::vector<bilingual_str>& warnings);
}
#endif
