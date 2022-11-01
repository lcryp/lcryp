#ifndef LCRYP_UTIL_THREAD_H
#define LCRYP_UTIL_THREAD_H
#include <functional>
#include <string>
namespace util {
void TraceThread(std::string_view thread_name, std::function<void()> thread_func);
}
#endif
