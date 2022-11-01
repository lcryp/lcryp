#ifndef LCRYP_UTIL_SPANPARSING_H
#define LCRYP_UTIL_SPANPARSING_H
#include <span.h>
#include <string>
#include <string_view>
#include <vector>
namespace spanparsing {
bool Const(const std::string& str, Span<const char>& sp);
bool Func(const std::string& str, Span<const char>& sp);
Span<const char> Expr(Span<const char>& sp);
template <typename T = Span<const char>>
std::vector<T> Split(const Span<const char>& sp, std::string_view separators)
{
    std::vector<T> ret;
    auto it = sp.begin();
    auto start = it;
    while (it != sp.end()) {
        if (separators.find(*it) != std::string::npos) {
            ret.emplace_back(start, it);
            start = it + 1;
        }
        ++it;
    }
    ret.emplace_back(start, it);
    return ret;
}
template <typename T = Span<const char>>
std::vector<T> Split(const Span<const char>& sp, char sep)
{
    return Split<T>(sp, std::string_view{&sep, 1});
}
}
#endif
