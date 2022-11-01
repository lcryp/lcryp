#ifndef LCRYP_UTIL_STRENCODINGS_H
#define LCRYP_UTIL_STRENCODINGS_H
#include <span.h>
#include <util/string.h>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <vector>
enum SafeChars
{
    SAFE_CHARS_DEFAULT,
    SAFE_CHARS_UA_COMMENT,
    SAFE_CHARS_FILENAME,
    SAFE_CHARS_URI,
};
enum class ByteUnit : uint64_t {
    NOOP = 1ULL,
    k = 1000ULL,
    K = 1024ULL,
    m = 1'000'000ULL,
    M = 1ULL << 20,
    g = 1'000'000'000ULL,
    G = 1ULL << 30,
    t = 1'000'000'000'000ULL,
    T = 1ULL << 40,
};
/**
* Remove unsafe chars. Safe chars chosen to allow simple messages/URLs/email
* addresses, but avoid anything even possibly remotely dangerous like & or >
* @param[in] str    The string to sanitize
* @param[in] rule   The set of safe chars to choose (default: least restrictive)
* @return           A new string without unsafe chars
*/
std::string SanitizeString(std::string_view str, int rule = SAFE_CHARS_DEFAULT);
/** Parse the hex string into bytes (uint8_t or std::byte). Ignores whitespace. */
template <typename Byte = uint8_t>
std::vector<Byte> ParseHex(std::string_view str);
signed char HexDigit(char c);
/* Returns true if each character in str is a hex character, and has an even
 * number of hex digits.*/
bool IsHex(std::string_view str);
/**
* Return true if the string is a hex number, optionally prefixed with "0x"
*/
bool IsHexNumber(std::string_view str);
std::optional<std::vector<unsigned char>> DecodeBase64(std::string_view str);
std::string EncodeBase64(Span<const unsigned char> input);
inline std::string EncodeBase64(Span<const std::byte> input) { return EncodeBase64(MakeUCharSpan(input)); }
inline std::string EncodeBase64(std::string_view str) { return EncodeBase64(MakeUCharSpan(str)); }
std::optional<std::vector<unsigned char>> DecodeBase32(std::string_view str);
/**
 * Base32 encode.
 * If `pad` is true, then the output will be padded with '=' so that its length
 * is a multiple of 8.
 */
std::string EncodeBase32(Span<const unsigned char> input, bool pad = true);
/**
 * Base32 encode.
 * If `pad` is true, then the output will be padded with '=' so that its length
 * is a multiple of 8.
 */
std::string EncodeBase32(std::string_view str, bool pad = true);
void SplitHostPort(std::string_view in, uint16_t& portOut, std::string& hostOut);
// LocaleIndependentAtoi is provided for backwards compatibility reasons.
//
// New code should use ToIntegral or the ParseInt* functions
// which provide parse error feedback.
//
// The goal of LocaleIndependentAtoi is to replicate the defined behaviour of
// std::atoi as it behaves under the "C" locale, and remove some undefined
// behavior. If the parsed value is bigger than the integer type's maximum
template <typename T>
T LocaleIndependentAtoi(std::string_view str)
{
    static_assert(std::is_integral<T>::value);
    T result;
    std::string_view s = TrimStringView(str);
    if (!s.empty() && s[0] == '+') {
        if (s.length() >= 2 && s[1] == '-') {
            return 0;
        }
        s = s.substr(1);
    }
    auto [_, error_condition] = std::from_chars(s.data(), s.data() + s.size(), result);
    if (error_condition == std::errc::result_out_of_range) {
        if (s.length() >= 1 && s[0] == '-') {
            return std::numeric_limits<T>::min();
        } else {
            return std::numeric_limits<T>::max();
        }
    } else if (error_condition != std::errc{}) {
        return 0;
    }
    return result;
}
constexpr bool IsDigit(char c)
{
    return c >= '0' && c <= '9';
}
constexpr inline bool IsSpace(char c) noexcept {
    return c == ' ' || c == '\f' || c == '\n' || c == '\r' || c == '\t' || c == '\v';
}
template <typename T>
std::optional<T> ToIntegral(std::string_view str)
{
    static_assert(std::is_integral<T>::value);
    T result;
    const auto [first_nonmatching, error_condition] = std::from_chars(str.data(), str.data() + str.size(), result);
    if (first_nonmatching != str.data() + str.size() || error_condition != std::errc{}) {
        return std::nullopt;
    }
    return result;
}
[[nodiscard]] bool ParseInt32(std::string_view str, int32_t *out);
[[nodiscard]] bool ParseInt64(std::string_view str, int64_t *out);
[[nodiscard]] bool ParseUInt8(std::string_view str, uint8_t *out);
[[nodiscard]] bool ParseUInt16(std::string_view str, uint16_t* out);
[[nodiscard]] bool ParseUInt32(std::string_view str, uint32_t *out);
[[nodiscard]] bool ParseUInt64(std::string_view str, uint64_t *out);
std::string HexStr(const Span<const uint8_t> s);
inline std::string HexStr(const Span<const char> s) { return HexStr(MakeUCharSpan(s)); }
inline std::string HexStr(const Span<const std::byte> s) { return HexStr(MakeUCharSpan(s)); }
std::string FormatParagraph(std::string_view in, size_t width = 79, size_t indent = 0);
template <typename T>
bool TimingResistantEqual(const T& a, const T& b)
{
    if (b.size() == 0) return a.size() == 0;
    size_t accumulator = a.size() ^ b.size();
    for (size_t i = 0; i < a.size(); i++)
        accumulator |= size_t(a[i] ^ b[i%b.size()]);
    return accumulator == 0;
}
[[nodiscard]] bool ParseFixedPoint(std::string_view, int decimals, int64_t *amount_out);
namespace {
struct IntIdentity
{
    [[maybe_unused]] int operator()(int x) const { return x; }
};
}
template<int frombits, int tobits, bool pad, typename O, typename It, typename I = IntIdentity>
bool ConvertBits(O outfn, It it, It end, I infn = {}) {
    size_t acc = 0;
    size_t bits = 0;
    constexpr size_t maxv = (1 << tobits) - 1;
    constexpr size_t max_acc = (1 << (frombits + tobits - 1)) - 1;
    while (it != end) {
        int v = infn(*it);
        if (v < 0) return false;
        acc = ((acc << frombits) | v) & max_acc;
        bits += frombits;
        while (bits >= tobits) {
            bits -= tobits;
            outfn((acc >> bits) & maxv);
        }
        ++it;
    }
    if (pad) {
        if (bits) outfn((acc << (tobits - bits)) & maxv);
    } else if (bits >= frombits || ((acc << (tobits - bits)) & maxv)) {
        return false;
    }
    return true;
}
constexpr char ToLower(char c)
{
    return (c >= 'A' && c <= 'Z' ? (c - 'A') + 'a' : c);
}
std::string ToLower(std::string_view str);
constexpr char ToUpper(char c)
{
    return (c >= 'a' && c <= 'z' ? (c - 'a') + 'A' : c);
}
std::string ToUpper(std::string_view str);
std::string Capitalize(std::string str);
std::optional<uint64_t> ParseByteUnits(std::string_view str, ByteUnit default_multiplier);
#endif
