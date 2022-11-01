#ifndef LCRYP_WALLET_SALVAGE_H
#define LCRYP_WALLET_SALVAGE_H
#include <fs.h>
#include <streams.h>
class ArgsManager;
struct bilingual_str;
namespace wallet {
bool RecoverDatabaseFile(const ArgsManager& args, const fs::path& file_path, bilingual_str& error, std::vector<bilingual_str>& warnings);
}
#endif
