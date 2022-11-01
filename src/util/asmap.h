#ifndef LCRYP_UTIL_ASMAP_H
#define LCRYP_UTIL_ASMAP_H
#include <fs.h>
#include <cstdint>
#include <vector>
uint32_t Interpret(const std::vector<bool> &asmap, const std::vector<bool> &ip);
bool SanityCheckASMap(const std::vector<bool>& asmap, int bits);
std::vector<bool> DecodeAsmap(fs::path path);
#endif
