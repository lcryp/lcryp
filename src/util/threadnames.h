#ifndef LCRYP_UTIL_THREADNAMES_H
#define LCRYP_UTIL_THREADNAMES_H
#include <string>
namespace util {
void ThreadRename(std::string&&);
void ThreadSetInternalName(std::string&&);
const std::string& ThreadGetInternalName();
}
#endif
