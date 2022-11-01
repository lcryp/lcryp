#ifndef LCRYP_UTIL_READWRITEFILE_H
#define LCRYP_UTIL_READWRITEFILE_H
#include <fs.h>
#include <limits>
#include <string>
#include <utility>
std::pair<bool,std::string> ReadBinaryFile(const fs::path &filename, size_t maxsize=std::numeric_limits<size_t>::max());
bool WriteBinaryFile(const fs::path &filename, const std::string &data);
#endif
