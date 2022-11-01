#ifndef LCRYP_UTIL_VECTOR_H
#define LCRYP_UTIL_VECTOR_H
#include <initializer_list>
#include <type_traits>
#include <utility>
#include <vector>
template<typename... Args>
inline std::vector<typename std::common_type<Args...>::type> Vector(Args&&... args)
{
    std::vector<typename std::common_type<Args...>::type> ret;
    ret.reserve(sizeof...(args));
    (void)std::initializer_list<int>{(ret.emplace_back(std::forward<Args>(args)), 0)...};
    return ret;
}
template<typename V>
inline V Cat(V v1, V&& v2)
{
    v1.reserve(v1.size() + v2.size());
    for (auto& arg : v2) {
        v1.push_back(std::move(arg));
    }
    return v1;
}
template<typename V>
inline V Cat(V v1, const V& v2)
{
    v1.reserve(v1.size() + v2.size());
    for (const auto& arg : v2) {
        v1.push_back(arg);
    }
    return v1;
}
#endif
