#ifndef LCRYP_SPAN_H
#define LCRYP_SPAN_H
#include <type_traits>
#include <cstddef>
#include <algorithm>
#include <assert.h>
#ifdef DEBUG
#define CONSTEXPR_IF_NOT_DEBUG
#define ASSERT_IF_DEBUG(x) assert((x))
#else
#define CONSTEXPR_IF_NOT_DEBUG constexpr
#define ASSERT_IF_DEBUG(x)
#endif
#if defined(__clang__)
#if __has_attribute(lifetimebound)
#define SPAN_ATTR_LIFETIMEBOUND [[clang::lifetimebound]]
#else
#define SPAN_ATTR_LIFETIMEBOUND
#endif
#else
#define SPAN_ATTR_LIFETIMEBOUND
#endif
template<typename C>
class Span
{
    C* m_data;
    std::size_t m_size;
    template <class T>
    struct is_Span_int : public std::false_type {};
    template <class T>
    struct is_Span_int<Span<T>> : public std::true_type {};
    template <class T>
    struct is_Span : public is_Span_int<typename std::remove_cv<T>::type>{};
public:
    constexpr Span() noexcept : m_data(nullptr), m_size(0) {}
    template <typename T, typename std::enable_if<std::is_convertible<T (*)[], C (*)[]>::value, int>::type = 0>
    constexpr Span(T* begin, std::size_t size) noexcept : m_data(begin), m_size(size) {}
    template <typename T, typename std::enable_if<std::is_convertible<T (*)[], C (*)[]>::value, int>::type = 0>
    CONSTEXPR_IF_NOT_DEBUG Span(T* begin, T* end) noexcept : m_data(begin), m_size(end - begin)
    {
        ASSERT_IF_DEBUG(end >= begin);
    }
    template <typename O, typename std::enable_if<std::is_convertible<O (*)[], C (*)[]>::value, int>::type = 0>
    constexpr Span(const Span<O>& other) noexcept : m_data(other.m_data), m_size(other.m_size) {}
    constexpr Span(const Span&) noexcept = default;
    Span& operator=(const Span& other) noexcept = default;
    template <int N>
    constexpr Span(C (&a)[N]) noexcept : m_data(a), m_size(N) {}
    template <typename V>
    constexpr Span(V& other SPAN_ATTR_LIFETIMEBOUND,
        typename std::enable_if<!is_Span<V>::value &&
                                std::is_convertible<typename std::remove_pointer<decltype(std::declval<V&>().data())>::type (*)[], C (*)[]>::value &&
                                std::is_convertible<decltype(std::declval<V&>().size()), std::size_t>::value, std::nullptr_t>::type = nullptr)
        : m_data(other.data()), m_size(other.size()){}
    template <typename V>
    constexpr Span(const V& other SPAN_ATTR_LIFETIMEBOUND,
        typename std::enable_if<!is_Span<V>::value &&
                                std::is_convertible<typename std::remove_pointer<decltype(std::declval<const V&>().data())>::type (*)[], C (*)[]>::value &&
                                std::is_convertible<decltype(std::declval<const V&>().size()), std::size_t>::value, std::nullptr_t>::type = nullptr)
        : m_data(other.data()), m_size(other.size()){}
    constexpr C* data() const noexcept { return m_data; }
    constexpr C* begin() const noexcept { return m_data; }
    constexpr C* end() const noexcept { return m_data + m_size; }
    CONSTEXPR_IF_NOT_DEBUG C& front() const noexcept
    {
        ASSERT_IF_DEBUG(size() > 0);
        return m_data[0];
    }
    CONSTEXPR_IF_NOT_DEBUG C& back() const noexcept
    {
        ASSERT_IF_DEBUG(size() > 0);
        return m_data[m_size - 1];
    }
    constexpr std::size_t size() const noexcept { return m_size; }
    constexpr std::size_t size_bytes() const noexcept { return sizeof(C) * m_size; }
    constexpr bool empty() const noexcept { return size() == 0; }
    CONSTEXPR_IF_NOT_DEBUG C& operator[](std::size_t pos) const noexcept
    {
        ASSERT_IF_DEBUG(size() > pos);
        return m_data[pos];
    }
    CONSTEXPR_IF_NOT_DEBUG Span<C> subspan(std::size_t offset) const noexcept
    {
        ASSERT_IF_DEBUG(size() >= offset);
        return Span<C>(m_data + offset, m_size - offset);
    }
    CONSTEXPR_IF_NOT_DEBUG Span<C> subspan(std::size_t offset, std::size_t count) const noexcept
    {
        ASSERT_IF_DEBUG(size() >= offset + count);
        return Span<C>(m_data + offset, count);
    }
    CONSTEXPR_IF_NOT_DEBUG Span<C> first(std::size_t count) const noexcept
    {
        ASSERT_IF_DEBUG(size() >= count);
        return Span<C>(m_data, count);
    }
    CONSTEXPR_IF_NOT_DEBUG Span<C> last(std::size_t count) const noexcept
    {
         ASSERT_IF_DEBUG(size() >= count);
         return Span<C>(m_data + m_size - count, count);
    }
    friend constexpr bool operator==(const Span& a, const Span& b) noexcept { return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin()); }
    friend constexpr bool operator!=(const Span& a, const Span& b) noexcept { return !(a == b); }
    friend constexpr bool operator<(const Span& a, const Span& b) noexcept { return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end()); }
    friend constexpr bool operator<=(const Span& a, const Span& b) noexcept { return !(b < a); }
    friend constexpr bool operator>(const Span& a, const Span& b) noexcept { return (b < a); }
    friend constexpr bool operator>=(const Span& a, const Span& b) noexcept { return !(a < b); }
    template <typename O> friend class Span;
};
template <typename T, typename EndOrSize> Span(T*, EndOrSize) -> Span<T>;
template <typename T, std::size_t N> Span(T (&)[N]) -> Span<T>;
template <typename T> Span(T&&) -> Span<std::enable_if_t<!std::is_lvalue_reference_v<T>, const std::remove_pointer_t<decltype(std::declval<T&&>().data())>>>;
template <typename T> Span(T&) -> Span<std::remove_pointer_t<decltype(std::declval<T&>().data())>>;
template <typename T>
T& SpanPopBack(Span<T>& span)
{
    size_t size = span.size();
    ASSERT_IF_DEBUG(size > 0);
    T& back = span[size - 1];
    span = Span<T>(span.data(), size - 1);
    return back;
}
inline const std::byte* AsBytePtr(const void* data) { return reinterpret_cast<const std::byte*>(data); }
inline std::byte* AsBytePtr(void* data) { return reinterpret_cast<std::byte*>(data); }
template <typename T>
Span<const std::byte> AsBytes(Span<T> s) noexcept
{
    return {AsBytePtr(s.data()), s.size_bytes()};
}
template <typename T>
Span<std::byte> AsWritableBytes(Span<T> s) noexcept
{
    return {AsBytePtr(s.data()), s.size_bytes()};
}
template <typename V>
Span<const std::byte> MakeByteSpan(V&& v) noexcept
{
    return AsBytes(Span{std::forward<V>(v)});
}
template <typename V>
Span<std::byte> MakeWritableByteSpan(V&& v) noexcept
{
    return AsWritableBytes(Span{std::forward<V>(v)});
}
inline unsigned char* UCharCast(char* c) { return (unsigned char*)c; }
inline unsigned char* UCharCast(unsigned char* c) { return c; }
inline const unsigned char* UCharCast(const char* c) { return (unsigned char*)c; }
inline const unsigned char* UCharCast(const unsigned char* c) { return c; }
inline const unsigned char* UCharCast(const std::byte* c) { return reinterpret_cast<const unsigned char*>(c); }
template <typename T> constexpr auto UCharSpanCast(Span<T> s) -> Span<typename std::remove_pointer<decltype(UCharCast(s.data()))>::type> { return {UCharCast(s.data()), s.size()}; }
template <typename V> constexpr auto MakeUCharSpan(V&& v) -> decltype(UCharSpanCast(Span{std::forward<V>(v)})) { return UCharSpanCast(Span{std::forward<V>(v)}); }
#endif
