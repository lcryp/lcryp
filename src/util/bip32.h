#ifndef LCRYP_UTIL_BIP32_H
#define LCRYP_UTIL_BIP32_H
#include <cstdint>
#include <string>
#include <vector>
[[nodiscard]] bool ParseHDKeypath(const std::string& keypath_str, std::vector<uint32_t>& keypath);
std::string WriteHDKeypath(const std::vector<uint32_t>& keypath);
std::string FormatHDKeypath(const std::vector<uint32_t>& path);
#endif
