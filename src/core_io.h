#ifndef LCRYP_CORE_IO_H
#define LCRYP_CORE_IO_H
#include <consensus/amount.h>
#include <string>
#include <vector>
class CBlock;
class CBlockHeader;
class CScript;
class CTransaction;
struct CMutableTransaction;
class uint256;
class UniValue;
class CTxUndo;
enum class TxVerbosity {
    SHOW_TXID,
    SHOW_DETAILS,
    SHOW_DETAILS_AND_PREVOUT
};
CScript ParseScript(const std::string& s);
std::string ScriptToAsmStr(const CScript& script, const bool fAttemptSighashDecode = false);
[[nodiscard]] bool DecodeHexTx(CMutableTransaction& tx, const std::string& hex_tx, bool try_no_witness = false, bool try_witness = true);
[[nodiscard]] bool DecodeHexBlk(CBlock&, const std::string& strHexBlk);
bool DecodeHexBlockHeader(CBlockHeader&, const std::string& hex_header);
bool ParseHashStr(const std::string& strHex, uint256& result);
std::vector<unsigned char> ParseHexUV(const UniValue& v, const std::string& strName);
int ParseSighashString(const UniValue& sighash);
UniValue ValueFromAmount(const CAmount amount);
std::string FormatScript(const CScript& script);
std::string EncodeHexTx(const CTransaction& tx, const int serializeFlags = 0);
std::string SighashToStr(unsigned char sighash_type);
void ScriptToUniv(const CScript& script, UniValue& out, bool include_hex = true, bool include_address = false);
void TxToUniv(const CTransaction& tx, const uint256& block_hash, UniValue& entry, bool include_hex = true, int serialize_flags = 0, const CTxUndo* txundo = nullptr, TxVerbosity verbosity = TxVerbosity::SHOW_DETAILS);
#endif
