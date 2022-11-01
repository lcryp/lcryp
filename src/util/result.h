#ifndef LCRYP_UTIL_RESULT_H
#define LCRYP_UTIL_RESULT_H
#include <attributes.h>
#include <util/translation.h>
#include <variant>
namespace util {
struct Error {
    bilingual_str message;
};
template <class T>
class Result
{
private:
    std::variant<bilingual_str, T> m_variant;
    template <typename FT>
    friend bilingual_str ErrorString(const Result<FT>& result);
public:
    Result(T obj) : m_variant{std::in_place_index_t<1>{}, std::move(obj)} {}
    Result(Error error) : m_variant{std::in_place_index_t<0>{}, std::move(error.message)} {}
    bool has_value() const noexcept { return m_variant.index() == 1; }
    const T& value() const LIFETIMEBOUND
    {
        assert(has_value());
        return std::get<1>(m_variant);
    }
    T& value() LIFETIMEBOUND
    {
        assert(has_value());
        return std::get<1>(m_variant);
    }
    template <class U>
    T value_or(U&& default_value) const&
    {
        return has_value() ? value() : std::forward<U>(default_value);
    }
    template <class U>
    T value_or(U&& default_value) &&
    {
        return has_value() ? std::move(value()) : std::forward<U>(default_value);
    }
    explicit operator bool() const noexcept { return has_value(); }
    const T* operator->() const LIFETIMEBOUND { return &value(); }
    const T& operator*() const LIFETIMEBOUND { return value(); }
    T* operator->() LIFETIMEBOUND { return &value(); }
    T& operator*() LIFETIMEBOUND { return value(); }
};
template <typename T>
bilingual_str ErrorString(const Result<T>& result)
{
    return result ? bilingual_str{} : std::get<0>(result.m_variant);
}
}
#endif
