#include <bech32.h>
#include <util/vector.h>
#include <array>
#include <assert.h>
#include <numeric>
#include <optional>
namespace bech32
{
namespace
{
typedef std::vector<uint8_t> data;
const char* CHARSET = "qpzry9x8gf2tvdw0s3jn54khce6mua7l";
const int8_t CHARSET_REV[128] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    15, -1, 10, 17, 21, 20, 26, 30,  7,  5, -1, -1, -1, -1, -1, -1,
    -1, 29, -1, 24, 13, 25,  9,  8, 23, -1, 18, 22, 31, 27, 19, -1,
     1,  0,  3, 16, 11, 28, 12, 14,  6,  4,  2, -1, -1, -1, -1, -1,
    -1, 29, -1, 24, 13, 25,  9,  8, 23, -1, 18, 22, 31, 27, 19, -1,
     1,  0,  3, 16, 11, 28, 12, 14,  6,  4,  2, -1, -1, -1, -1, -1
};
constexpr std::pair<std::array<int16_t, 1023>, std::array<int16_t, 1024>> GenerateGFTables()
{
    std::array<int8_t, 31> GF32_EXP{};
    std::array<int8_t, 32> GF32_LOG{};
    const int fmod = 41;
    GF32_EXP[0] = 1;
    GF32_LOG[0] = -1;
    GF32_LOG[1] = 0;
    int v = 1;
    for (int i = 1; i < 31; ++i) {
        v = v << 1;
        if (v & 32) v ^= fmod;
        GF32_EXP[i] = v;
        GF32_LOG[v] = i;
    }
    std::array<int16_t, 1023> GF1024_EXP{};
    std::array<int16_t, 1024> GF1024_LOG{};
    GF1024_EXP[0] = 1;
    GF1024_LOG[0] = -1;
    GF1024_LOG[1] = 0;
    v = 1;
    for (int i = 1; i < 1023; ++i) {
        int v0 = v & 31;
        int v1 = v >> 5;
        int v0n = v1 ? GF32_EXP.at((GF32_LOG.at(v1) + GF32_LOG.at(23)) % 31) : 0;
        int v1n = (v1 ? GF32_EXP.at((GF32_LOG.at(v1) + GF32_LOG.at(9)) % 31) : 0) ^ v0;
        v = v1n << 5 | v0n;
        GF1024_EXP[i] = v;
        GF1024_LOG[v] = i;
    }
    return std::make_pair(GF1024_EXP, GF1024_LOG);
}
constexpr auto tables = GenerateGFTables();
constexpr const std::array<int16_t, 1023>& GF1024_EXP = tables.first;
constexpr const std::array<int16_t, 1024>& GF1024_LOG = tables.second;
uint32_t EncodingConstant(Encoding encoding) {
    assert(encoding == Encoding::BECH32 || encoding == Encoding::BECH32M);
    return encoding == Encoding::BECH32 ? 1 : 0x2bc830a3;
}
uint32_t PolyMod(const data& v)
{
    uint32_t c = 1;
    for (const auto v_i : v) {
        uint8_t c0 = c >> 25;
        c = ((c & 0x1ffffff) << 5) ^ v_i;
        if (c0 & 1)  c ^= 0x3b6a57b2;
        if (c0 & 2)  c ^= 0x26508e6d;
        if (c0 & 4)  c ^= 0x1ea119fa;
        if (c0 & 8)  c ^= 0x3d4233dd;
        if (c0 & 16) c ^= 0x2a1462b3;
    }
    return c;
}
constexpr std::array<uint32_t, 25> GenerateSyndromeConstants() {
    std::array<uint32_t, 25> SYNDROME_CONSTS{};
    for (int k = 1; k < 6; ++k) {
        for (int shift = 0; shift < 5; ++shift) {
            int16_t b = GF1024_LOG.at(1 << shift);
            int16_t c0 = GF1024_EXP.at((997*k + b) % 1023);
            int16_t c1 = GF1024_EXP.at((998*k + b) % 1023);
            int16_t c2 = GF1024_EXP.at((999*k + b) % 1023);
            uint32_t c = c2 << 20 | c1 << 10 | c0;
            int ind = 5*(k-1) + shift;
            SYNDROME_CONSTS[ind] = c;
        }
    }
    return SYNDROME_CONSTS;
}
constexpr std::array<uint32_t, 25> SYNDROME_CONSTS = GenerateSyndromeConstants();
uint32_t Syndrome(const uint32_t residue) {
    uint32_t low = residue & 0x1f;
    uint32_t result = low ^ (low << 10) ^ (low << 20);
    for (int i = 0; i < 25; ++i) {
        result ^= ((residue >> (5+i)) & 1 ? SYNDROME_CONSTS.at(i) : 0);
    }
    return result;
}
inline unsigned char LowerCase(unsigned char c)
{
    return (c >= 'A' && c <= 'Z') ? (c - 'A') + 'a' : c;
}
bool CheckCharacters(const std::string& str, std::vector<int>& errors)
{
    bool lower = false, upper = false;
    for (size_t i = 0; i < str.size(); ++i) {
        unsigned char c{(unsigned char)(str[i])};
        if (c >= 'a' && c <= 'z') {
            if (upper) {
                errors.push_back(i);
            } else {
                lower = true;
            }
        } else if (c >= 'A' && c <= 'Z') {
            if (lower) {
                errors.push_back(i);
            } else {
                upper = true;
            }
        } else if (c < 33 || c > 126) {
            errors.push_back(i);
        }
    }
    return errors.empty();
}
data ExpandHRP(const std::string& hrp)
{
    data ret;
    ret.reserve(hrp.size() + 90);
    ret.resize(hrp.size() * 2 + 1);
    for (size_t i = 0; i < hrp.size(); ++i) {
        unsigned char c = hrp[i];
        ret[i] = c >> 5;
        ret[i + hrp.size() + 1] = c & 0x1f;
    }
    ret[hrp.size()] = 0;
    return ret;
}
Encoding VerifyChecksum(const std::string& hrp, const data& values)
{
    const uint32_t check = PolyMod(Cat(ExpandHRP(hrp), values));
    if (check == EncodingConstant(Encoding::BECH32)) return Encoding::BECH32;
    if (check == EncodingConstant(Encoding::BECH32M)) return Encoding::BECH32M;
    return Encoding::INVALID;
}
data CreateChecksum(Encoding encoding, const std::string& hrp, const data& values)
{
    data enc = Cat(ExpandHRP(hrp), values);
    enc.resize(enc.size() + 6);
    uint32_t mod = PolyMod(enc) ^ EncodingConstant(encoding);
    data ret(6);
    for (size_t i = 0; i < 6; ++i) {
        ret[i] = (mod >> (5 * (5 - i))) & 31;
    }
    return ret;
}
}
std::string Encode(Encoding encoding, const std::string& hrp, const data& values) {
    for (const char& c : hrp) assert(c < 'A' || c > 'Z');
    data checksum = CreateChecksum(encoding, hrp, values);
    data combined = Cat(values, checksum);
    std::string ret = hrp + '1';
    ret.reserve(ret.size() + combined.size());
    for (const auto c : combined) {
        ret += CHARSET[c];
    }
    return ret;
}
DecodeResult Decode(const std::string& str) {
    std::vector<int> errors;
    if (!CheckCharacters(str, errors)) return {};
    size_t pos = str.rfind('1');
    if (str.size() > 90 || pos == str.npos || pos == 0 || pos + 7 > str.size()) {
        return {};
    }
    data values(str.size() - 1 - pos);
    for (size_t i = 0; i < str.size() - 1 - pos; ++i) {
        unsigned char c = str[i + pos + 1];
        int8_t rev = CHARSET_REV[c];
        if (rev == -1) {
            return {};
        }
        values[i] = rev;
    }
    std::string hrp;
    for (size_t i = 0; i < pos; ++i) {
        hrp += LowerCase(str[i]);
    }
    Encoding result = VerifyChecksum(hrp, values);
    if (result == Encoding::INVALID) return {};
    return {result, std::move(hrp), data(values.begin(), values.end() - 6)};
}
std::pair<std::string, std::vector<int>> LocateErrors(const std::string& str) {
    std::vector<int> error_locations{};
    if (str.size() > 90) {
        error_locations.resize(str.size() - 90);
        std::iota(error_locations.begin(), error_locations.end(), 90);
        return std::make_pair("Bech32 string too long", std::move(error_locations));
    }
    if (!CheckCharacters(str, error_locations)){
        return std::make_pair("Invalid character or mixed case", std::move(error_locations));
    }
    size_t pos = str.rfind('1');
    if (pos == str.npos) {
        return std::make_pair("Missing separator", std::vector<int>{});
    }
    if (pos == 0 || pos + 7 > str.size()) {
        error_locations.push_back(pos);
        return std::make_pair("Invalid separator position", std::move(error_locations));
    }
    std::string hrp;
    for (size_t i = 0; i < pos; ++i) {
        hrp += LowerCase(str[i]);
    }
    size_t length = str.size() - 1 - pos;
    data values(length);
    for (size_t i = pos + 1; i < str.size(); ++i) {
        unsigned char c = str[i];
        int8_t rev = CHARSET_REV[c];
        if (rev == -1) {
            error_locations.push_back(i);
            return std::make_pair("Invalid Base 32 character", std::move(error_locations));
        }
        values[i - pos - 1] = rev;
    }
    std::optional<Encoding> error_encoding;
    for (Encoding encoding : {Encoding::BECH32, Encoding::BECH32M}) {
        std::vector<int> possible_errors;
        uint32_t residue = PolyMod(Cat(ExpandHRP(hrp), values)) ^ EncodingConstant(encoding);
        if (residue != 0) {
            uint32_t syn = Syndrome(residue);
            int s0 = syn & 0x3FF;
            int s1 = (syn >> 10) & 0x3FF;
            int s2 = syn >> 20;
            int l_s0 = GF1024_LOG.at(s0);
            int l_s1 = GF1024_LOG.at(s1);
            int l_s2 = GF1024_LOG.at(s2);
            if (l_s0 != -1 && l_s1 != -1 && l_s2 != -1 && (2 * l_s1 - l_s2 - l_s0 + 2046) % 1023 == 0) {
                size_t p1 = (l_s1 - l_s0 + 1023) % 1023;
                int l_e1 = l_s0 + (1023 - 997) * p1;
                if (p1 < length && !(l_e1 % 33)) {
                    possible_errors.push_back(str.size() - p1 - 1);
                }
            } else {
                for (size_t p1 = 0; p1 < length; ++p1) {
                    int s2_s1p1 = s2 ^ (s1 == 0 ? 0 : GF1024_EXP.at((l_s1 + p1) % 1023));
                    if (s2_s1p1 == 0) continue;
                    int l_s2_s1p1 = GF1024_LOG.at(s2_s1p1);
                    int s1_s0p1 = s1 ^ (s0 == 0 ? 0 : GF1024_EXP.at((l_s0 + p1) % 1023));
                    if (s1_s0p1 == 0) continue;
                    int l_s1_s0p1 = GF1024_LOG.at(s1_s0p1);
                    size_t p2 = (l_s2_s1p1 - l_s1_s0p1 + 1023) % 1023;
                    if (p2 >= length || p1 == p2) continue;
                    int s1_s0p2 = s1 ^ (s0 == 0 ? 0 : GF1024_EXP.at((l_s0 + p2) % 1023));
                    if (s1_s0p2 == 0) continue;
                    int l_s1_s0p2 = GF1024_LOG.at(s1_s0p2);
                    int inv_p1_p2 = 1023 - GF1024_LOG.at(GF1024_EXP.at(p1) ^ GF1024_EXP.at(p2));
                    int l_e2 = l_s1_s0p1 + inv_p1_p2 + (1023 - 997) * p2;
                    if (l_e2 % 33) continue;
                    int l_e1 = l_s1_s0p2 + inv_p1_p2 + (1023 - 997) * p1;
                    if (l_e1 % 33) continue;
                    if (p1 > p2) {
                        possible_errors.push_back(str.size() - p1 - 1);
                        possible_errors.push_back(str.size() - p2 - 1);
                    } else {
                        possible_errors.push_back(str.size() - p2 - 1);
                        possible_errors.push_back(str.size() - p1 - 1);
                    }
                    break;
                }
            }
        } else {
            return std::make_pair("", std::vector<int>{});
        }
        if (error_locations.empty() || (!possible_errors.empty() && possible_errors.size() < error_locations.size())) {
            error_locations = std::move(possible_errors);
            if (!error_locations.empty()) error_encoding = encoding;
        }
    }
    std::string error_message = error_encoding == Encoding::BECH32M ? "Invalid Bech32m checksum"
                              : error_encoding == Encoding::BECH32 ? "Invalid Bech32 checksum"
                              : "Invalid checksum";
    return std::make_pair(error_message, std::move(error_locations));
}
}
