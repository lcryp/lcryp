#ifndef LCRYP_BECH32_H
#define LCRYP_BECH32_H
#include <stdint.h>
#include <string>
#include <vector>
namespace bech32
{
enum class Encoding {
    INVALID,
    BECH32,
    BECH32M,
};
std::string Encode(Encoding encoding, const std::string& hrp, const std::vector<uint8_t>& values);
struct DecodeResult
{
    Encoding encoding;
    std::string hrp;
    std::vector<uint8_t> data;
    DecodeResult() : encoding(Encoding::INVALID) {}
    DecodeResult(Encoding enc, std::string&& h, std::vector<uint8_t>&& d) : encoding(enc), hrp(std::move(h)), data(std::move(d)) {}
};
DecodeResult Decode(const std::string& str);
std::pair<std::string, std::vector<int>> LocateErrors(const std::string& str);
}
#endif
