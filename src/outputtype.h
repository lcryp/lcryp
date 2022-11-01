#ifndef LCRYP_OUTPUTTYPE_H
#define LCRYP_OUTPUTTYPE_H
#include <script/signingprovider.h>
#include <script/standard.h>
#include <array>
#include <optional>
#include <string>
#include <vector>
enum class OutputType {
    LEGACY,
    P2SH_SEGWIT,
    BECH32,
    BECH32M,
    UNKNOWN,
};
static constexpr auto OUTPUT_TYPES = std::array{
    OutputType::LEGACY,
    OutputType::P2SH_SEGWIT,
    OutputType::BECH32,
    OutputType::BECH32M,
};
std::optional<OutputType> ParseOutputType(const std::string& str);
const std::string& FormatOutputType(OutputType type);
CTxDestination GetDestinationForKey(const CPubKey& key, OutputType);
std::vector<CTxDestination> GetAllDestinationsForKey(const CPubKey& key);
CTxDestination AddAndGetDestinationForScript(FillableSigningProvider& keystore, const CScript& script, OutputType);
std::optional<OutputType> OutputTypeFromDestination(const CTxDestination& dest);
#endif
