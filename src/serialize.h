#ifndef LCRYP_SERIALIZE_H
#define LCRYP_SERIALIZE_H
#include <compat/endian.h>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <ios>
#include <limits>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <string.h>
#include <utility>
#include <vector>
#include <prevector.h>
#include <span.h>
static constexpr uint64_t MAX_SIZE = 0x02000000;
static const unsigned int MAX_VECTOR_ALLOCATE = 5000000;
struct deserialize_type {};
constexpr deserialize_type deserialize {};
template<typename Stream> inline void ser_writedata8(Stream &s, uint8_t obj)
{
    s.write(AsBytes(Span{&obj, 1}));
}
template<typename Stream> inline void ser_writedata16(Stream &s, uint16_t obj)
{
    obj = htole16(obj);
    s.write(AsBytes(Span{&obj, 1}));
}
template<typename Stream> inline void ser_writedata16be(Stream &s, uint16_t obj)
{
    obj = htobe16(obj);
    s.write(AsBytes(Span{&obj, 1}));
}
template<typename Stream> inline void ser_writedata32(Stream &s, uint32_t obj)
{
    obj = htole32(obj);
    s.write(AsBytes(Span{&obj, 1}));
}
template<typename Stream> inline void ser_writedata32be(Stream &s, uint32_t obj)
{
    obj = htobe32(obj);
    s.write(AsBytes(Span{&obj, 1}));
}
template<typename Stream> inline void ser_writedata64(Stream &s, uint64_t obj)
{
    obj = htole64(obj);
    s.write(AsBytes(Span{&obj, 1}));
}
template<typename Stream> inline uint8_t ser_readdata8(Stream &s)
{
    uint8_t obj;
    s.read(AsWritableBytes(Span{&obj, 1}));
    return obj;
}
template<typename Stream> inline uint16_t ser_readdata16(Stream &s)
{
    uint16_t obj;
    s.read(AsWritableBytes(Span{&obj, 1}));
    return le16toh(obj);
}
template<typename Stream> inline uint16_t ser_readdata16be(Stream &s)
{
    uint16_t obj;
    s.read(AsWritableBytes(Span{&obj, 1}));
    return be16toh(obj);
}
template<typename Stream> inline uint32_t ser_readdata32(Stream &s)
{
    uint32_t obj;
    s.read(AsWritableBytes(Span{&obj, 1}));
    return le32toh(obj);
}
template<typename Stream> inline uint32_t ser_readdata32be(Stream &s)
{
    uint32_t obj;
    s.read(AsWritableBytes(Span{&obj, 1}));
    return be32toh(obj);
}
template<typename Stream> inline uint64_t ser_readdata64(Stream &s)
{
    uint64_t obj;
    s.read(AsWritableBytes(Span{&obj, 1}));
    return le64toh(obj);
}
class CSizeComputer;
enum
{
    SER_NETWORK         = (1 << 0),
    SER_DISK            = (1 << 1),
    SER_GETHASH         = (1 << 2),
};
template<typename X> X& ReadWriteAsHelper(X& x) { return x; }
template<typename X> const X& ReadWriteAsHelper(const X& x) { return x; }
#define READWRITE(...) (::SerReadWriteMany(s, ser_action, __VA_ARGS__))
#define READWRITEAS(type, obj) (::SerReadWriteMany(s, ser_action, ReadWriteAsHelper<type>(obj)))
#define SER_READ(obj, code) ::SerRead(s, ser_action, obj, [&](Stream& s, typename std::remove_const<Type>::type& obj) { code; })
#define SER_WRITE(obj, code) ::SerWrite(s, ser_action, obj, [&](Stream& s, const Type& obj) { code; })
#define FORMATTER_METHODS(cls, obj) \
    template<typename Stream> \
    static void Ser(Stream& s, const cls& obj) { SerializationOps(obj, s, CSerActionSerialize()); } \
    template<typename Stream> \
    static void Unser(Stream& s, cls& obj) { SerializationOps(obj, s, CSerActionUnserialize()); } \
    template<typename Stream, typename Type, typename Operation> \
    static inline void SerializationOps(Type& obj, Stream& s, Operation ser_action) \


#define SERIALIZE_METHODS(cls, obj)                                                 \
    template<typename Stream>                                                       \
    void Serialize(Stream& s) const                                                 \
    {                                                                               \
        static_assert(std::is_same<const cls&, decltype(*this)>::value, "Serialize type mismatch"); \
        Ser(s, *this);                                                              \
    }                                                                               \
    template<typename Stream>                                                       \
    void Unserialize(Stream& s)                                                     \
    {                                                                               \
        static_assert(std::is_same<cls&, decltype(*this)>::value, "Unserialize type mismatch"); \
        Unser(s, *this);                                                            \
    }                                                                               \
    FORMATTER_METHODS(cls, obj)
#ifndef CHAR_EQUALS_INT8
template <typename Stream> void Serialize(Stream&, char) = delete;
#endif
template<typename Stream> inline void Serialize(Stream& s, int8_t a  ) { ser_writedata8(s, a); }
template<typename Stream> inline void Serialize(Stream& s, uint8_t a ) { ser_writedata8(s, a); }
template<typename Stream> inline void Serialize(Stream& s, int16_t a ) { ser_writedata16(s, a); }
template<typename Stream> inline void Serialize(Stream& s, uint16_t a) { ser_writedata16(s, a); }
template<typename Stream> inline void Serialize(Stream& s, int32_t a ) { ser_writedata32(s, a); }
template<typename Stream> inline void Serialize(Stream& s, uint32_t a) { ser_writedata32(s, a); }
template<typename Stream> inline void Serialize(Stream& s, int64_t a ) { ser_writedata64(s, a); }
template<typename Stream> inline void Serialize(Stream& s, uint64_t a) { ser_writedata64(s, a); }
template<typename Stream, int N> inline void Serialize(Stream& s, const char (&a)[N]) { s.write(MakeByteSpan(a)); }
template<typename Stream, int N> inline void Serialize(Stream& s, const unsigned char (&a)[N]) { s.write(MakeByteSpan(a)); }
template<typename Stream> inline void Serialize(Stream& s, const Span<const unsigned char>& span) { s.write(AsBytes(span)); }
template<typename Stream> inline void Serialize(Stream& s, const Span<unsigned char>& span) { s.write(AsBytes(span)); }
#ifndef CHAR_EQUALS_INT8
template <typename Stream> void Unserialize(Stream&, char) = delete;
#endif
template<typename Stream> inline void Unserialize(Stream& s, int8_t& a  ) { a = ser_readdata8(s); }
template<typename Stream> inline void Unserialize(Stream& s, uint8_t& a ) { a = ser_readdata8(s); }
template<typename Stream> inline void Unserialize(Stream& s, int16_t& a ) { a = ser_readdata16(s); }
template<typename Stream> inline void Unserialize(Stream& s, uint16_t& a) { a = ser_readdata16(s); }
template<typename Stream> inline void Unserialize(Stream& s, int32_t& a ) { a = ser_readdata32(s); }
template<typename Stream> inline void Unserialize(Stream& s, uint32_t& a) { a = ser_readdata32(s); }
template<typename Stream> inline void Unserialize(Stream& s, int64_t& a ) { a = ser_readdata64(s); }
template<typename Stream> inline void Unserialize(Stream& s, uint64_t& a) { a = ser_readdata64(s); }
template<typename Stream, int N> inline void Unserialize(Stream& s, char (&a)[N]) { s.read(MakeWritableByteSpan(a)); }
template<typename Stream, int N> inline void Unserialize(Stream& s, unsigned char (&a)[N]) { s.read(MakeWritableByteSpan(a)); }
template<typename Stream> inline void Unserialize(Stream& s, Span<unsigned char>& span) { s.read(AsWritableBytes(span)); }
template <typename Stream> inline void Serialize(Stream& s, bool a) { uint8_t f = a; ser_writedata8(s, f); }
template <typename Stream> inline void Unserialize(Stream& s, bool& a) { uint8_t f = ser_readdata8(s); a = f; }
inline unsigned int GetSizeOfCompactSize(uint64_t nSize)
{
    if (nSize < 253)             return sizeof(unsigned char);
    else if (nSize <= std::numeric_limits<uint16_t>::max()) return sizeof(unsigned char) + sizeof(uint16_t);
    else if (nSize <= std::numeric_limits<unsigned int>::max())  return sizeof(unsigned char) + sizeof(unsigned int);
    else                         return sizeof(unsigned char) + sizeof(uint64_t);
}
inline void WriteCompactSize(CSizeComputer& os, uint64_t nSize);
template<typename Stream>
void WriteCompactSize(Stream& os, uint64_t nSize)
{
    if (nSize < 253)
    {
        ser_writedata8(os, nSize);
    }
    else if (nSize <= std::numeric_limits<uint16_t>::max())
    {
        ser_writedata8(os, 253);
        ser_writedata16(os, nSize);
    }
    else if (nSize <= std::numeric_limits<unsigned int>::max())
    {
        ser_writedata8(os, 254);
        ser_writedata32(os, nSize);
    }
    else
    {
        ser_writedata8(os, 255);
        ser_writedata64(os, nSize);
    }
    return;
}
template<typename Stream>
uint64_t ReadCompactSize(Stream& is, bool range_check = true)
{
    uint8_t chSize = ser_readdata8(is);
    uint64_t nSizeRet = 0;
    if (chSize < 253)
    {
        nSizeRet = chSize;
    }
    else if (chSize == 253)
    {
        nSizeRet = ser_readdata16(is);
        if (nSizeRet < 253)
            throw std::ios_base::failure("non-canonical ReadCompactSize()");
    }
    else if (chSize == 254)
    {
        nSizeRet = ser_readdata32(is);
        if (nSizeRet < 0x10000u)
            throw std::ios_base::failure("non-canonical ReadCompactSize()");
    }
    else
    {
        nSizeRet = ser_readdata64(is);
        if (nSizeRet < 0x100000000ULL)
            throw std::ios_base::failure("non-canonical ReadCompactSize()");
    }
    if (range_check && nSizeRet > MAX_SIZE) {
        throw std::ios_base::failure("ReadCompactSize(): size too large");
    }
    return nSizeRet;
}
enum class VarIntMode { DEFAULT, NONNEGATIVE_SIGNED };
template <VarIntMode Mode, typename I>
struct CheckVarIntMode {
    constexpr CheckVarIntMode()
    {
        static_assert(Mode != VarIntMode::DEFAULT || std::is_unsigned<I>::value, "Unsigned type required with mode DEFAULT.");
        static_assert(Mode != VarIntMode::NONNEGATIVE_SIGNED || std::is_signed<I>::value, "Signed type required with mode NONNEGATIVE_SIGNED.");
    }
};
template<VarIntMode Mode, typename I>
inline unsigned int GetSizeOfVarInt(I n)
{
    CheckVarIntMode<Mode, I>();
    int nRet = 0;
    while(true) {
        nRet++;
        if (n <= 0x7F)
            break;
        n = (n >> 7) - 1;
    }
    return nRet;
}
template<typename I>
inline void WriteVarInt(CSizeComputer& os, I n);
template<typename Stream, VarIntMode Mode, typename I>
void WriteVarInt(Stream& os, I n)
{
    CheckVarIntMode<Mode, I>();
    unsigned char tmp[(sizeof(n)*8+6)/7];
    int len=0;
    while(true) {
        tmp[len] = (n & 0x7F) | (len ? 0x80 : 0x00);
        if (n <= 0x7F)
            break;
        n = (n >> 7) - 1;
        len++;
    }
    do {
        ser_writedata8(os, tmp[len]);
    } while(len--);
}
template<typename Stream, VarIntMode Mode, typename I>
I ReadVarInt(Stream& is)
{
    CheckVarIntMode<Mode, I>();
    I n = 0;
    while(true) {
        unsigned char chData = ser_readdata8(is);
        if (n > (std::numeric_limits<I>::max() >> 7)) {
           throw std::ios_base::failure("ReadVarInt(): size too large");
        }
        n = (n << 7) | (chData & 0x7F);
        if (chData & 0x80) {
            if (n == std::numeric_limits<I>::max()) {
                throw std::ios_base::failure("ReadVarInt(): size too large");
            }
            n++;
        } else {
            return n;
        }
    }
}
template<typename Formatter, typename T>
class Wrapper
{
    static_assert(std::is_lvalue_reference<T>::value, "Wrapper needs an lvalue reference type T");
protected:
    T m_object;
public:
    explicit Wrapper(T obj) : m_object(obj) {}
    template<typename Stream> void Serialize(Stream &s) const { Formatter().Ser(s, m_object); }
    template<typename Stream> void Unserialize(Stream &s) { Formatter().Unser(s, m_object); }
};
template<typename Formatter, typename T>
static inline Wrapper<Formatter, T&> Using(T&& t) { return Wrapper<Formatter, T&>(t); }
#define VARINT_MODE(obj, mode) Using<VarIntFormatter<mode>>(obj)
#define VARINT(obj) Using<VarIntFormatter<VarIntMode::DEFAULT>>(obj)
#define COMPACTSIZE(obj) Using<CompactSizeFormatter<true>>(obj)
#define LIMITED_STRING(obj,n) Using<LimitedStringFormatter<n>>(obj)
template<VarIntMode Mode>
struct VarIntFormatter
{
    template<typename Stream, typename I> void Ser(Stream &s, I v)
    {
        WriteVarInt<Stream,Mode,typename std::remove_cv<I>::type>(s, v);
    }
    template<typename Stream, typename I> void Unser(Stream& s, I& v)
    {
        v = ReadVarInt<Stream,Mode,typename std::remove_cv<I>::type>(s);
    }
};
template<int Bytes, bool BigEndian = false>
struct CustomUintFormatter
{
    static_assert(Bytes > 0 && Bytes <= 8, "CustomUintFormatter Bytes out of range");
    static constexpr uint64_t MAX = 0xffffffffffffffff >> (8 * (8 - Bytes));
    template <typename Stream, typename I> void Ser(Stream& s, I v)
    {
        if (v < 0 || v > MAX) throw std::ios_base::failure("CustomUintFormatter value out of range");
        if (BigEndian) {
            uint64_t raw = htobe64(v);
            s.write({AsBytePtr(&raw) + 8 - Bytes, Bytes});
        } else {
            uint64_t raw = htole64(v);
            s.write({AsBytePtr(&raw), Bytes});
        }
    }
    template <typename Stream, typename I> void Unser(Stream& s, I& v)
    {
        using U = typename std::conditional<std::is_enum<I>::value, std::underlying_type<I>, std::common_type<I>>::type::type;
        static_assert(std::numeric_limits<U>::max() >= MAX && std::numeric_limits<U>::min() <= 0, "Assigned type too small");
        uint64_t raw = 0;
        if (BigEndian) {
            s.read({AsBytePtr(&raw) + 8 - Bytes, Bytes});
            v = static_cast<I>(be64toh(raw));
        } else {
            s.read({AsBytePtr(&raw), Bytes});
            v = static_cast<I>(le64toh(raw));
        }
    }
};
template<int Bytes> using BigEndianFormatter = CustomUintFormatter<Bytes, true>;
template<bool RangeCheck>
struct CompactSizeFormatter
{
    template<typename Stream, typename I>
    void Unser(Stream& s, I& v)
    {
        uint64_t n = ReadCompactSize<Stream>(s, RangeCheck);
        if (n < std::numeric_limits<I>::min() || n > std::numeric_limits<I>::max()) {
            throw std::ios_base::failure("CompactSize exceeds limit of type");
        }
        v = n;
    }
    template<typename Stream, typename I>
    void Ser(Stream& s, I v)
    {
        static_assert(std::is_unsigned<I>::value, "CompactSize only supported for unsigned integers");
        static_assert(std::numeric_limits<I>::max() <= std::numeric_limits<uint64_t>::max(), "CompactSize only supports 64-bit integers and below");
        WriteCompactSize<Stream>(s, v);
    }
};
template <typename U, bool LOSSY = false>
struct ChronoFormatter {
    template <typename Stream, typename Tp>
    void Unser(Stream& s, Tp& tp)
    {
        U u;
        s >> u;
        tp = Tp{typename Tp::duration{typename Tp::duration::rep{u}}};
    }
    template <typename Stream, typename Tp>
    void Ser(Stream& s, Tp tp)
    {
        if constexpr (LOSSY) {
            s << U(tp.time_since_epoch().count());
        } else {
            s << U{tp.time_since_epoch().count()};
        }
    }
};
template <typename U>
using LossyChronoFormatter = ChronoFormatter<U, true>;
class CompactSizeWriter
{
protected:
    uint64_t n;
public:
    explicit CompactSizeWriter(uint64_t n_in) : n(n_in) { }
    template<typename Stream>
    void Serialize(Stream &s) const {
        WriteCompactSize<Stream>(s, n);
    }
};
template<size_t Limit>
struct LimitedStringFormatter
{
    template<typename Stream>
    void Unser(Stream& s, std::string& v)
    {
        size_t size = ReadCompactSize(s);
        if (size > Limit) {
            throw std::ios_base::failure("String length limit exceeded");
        }
        v.resize(size);
        if (size != 0) s.read(MakeWritableByteSpan(v));
    }
    template<typename Stream>
    void Ser(Stream& s, const std::string& v)
    {
        s << v;
    }
};
template<class Formatter>
struct VectorFormatter
{
    template<typename Stream, typename V>
    void Ser(Stream& s, const V& v)
    {
        Formatter formatter;
        WriteCompactSize(s, v.size());
        for (const typename V::value_type& elem : v) {
            formatter.Ser(s, elem);
        }
    }
    template<typename Stream, typename V>
    void Unser(Stream& s, V& v)
    {
        Formatter formatter;
        v.clear();
        size_t size = ReadCompactSize(s);
        size_t allocated = 0;
        while (allocated < size) {
            static_assert(sizeof(typename V::value_type) <= MAX_VECTOR_ALLOCATE, "Vector element size too large");
            allocated = std::min(size, allocated + MAX_VECTOR_ALLOCATE / sizeof(typename V::value_type));
            v.reserve(allocated);
            while (v.size() < allocated) {
                v.emplace_back();
                formatter.Unser(s, v.back());
            }
        }
    };
};
template<typename Stream, typename C> void Serialize(Stream& os, const std::basic_string<C>& str);
template<typename Stream, typename C> void Unserialize(Stream& is, std::basic_string<C>& str);
template<typename Stream, unsigned int N, typename T> void Serialize_impl(Stream& os, const prevector<N, T>& v, const unsigned char&);
template<typename Stream, unsigned int N, typename T, typename V> void Serialize_impl(Stream& os, const prevector<N, T>& v, const V&);
template<typename Stream, unsigned int N, typename T> inline void Serialize(Stream& os, const prevector<N, T>& v);
template<typename Stream, unsigned int N, typename T> void Unserialize_impl(Stream& is, prevector<N, T>& v, const unsigned char&);
template<typename Stream, unsigned int N, typename T, typename V> void Unserialize_impl(Stream& is, prevector<N, T>& v, const V&);
template<typename Stream, unsigned int N, typename T> inline void Unserialize(Stream& is, prevector<N, T>& v);
template<typename Stream, typename T, typename A> void Serialize_impl(Stream& os, const std::vector<T, A>& v, const unsigned char&);
template<typename Stream, typename T, typename A> void Serialize_impl(Stream& os, const std::vector<T, A>& v, const bool&);
template<typename Stream, typename T, typename A, typename V> void Serialize_impl(Stream& os, const std::vector<T, A>& v, const V&);
template<typename Stream, typename T, typename A> inline void Serialize(Stream& os, const std::vector<T, A>& v);
template<typename Stream, typename T, typename A> void Unserialize_impl(Stream& is, std::vector<T, A>& v, const unsigned char&);
template<typename Stream, typename T, typename A, typename V> void Unserialize_impl(Stream& is, std::vector<T, A>& v, const V&);
template<typename Stream, typename T, typename A> inline void Unserialize(Stream& is, std::vector<T, A>& v);
template<typename Stream, typename K, typename T> void Serialize(Stream& os, const std::pair<K, T>& item);
template<typename Stream, typename K, typename T> void Unserialize(Stream& is, std::pair<K, T>& item);
template<typename Stream, typename K, typename T, typename Pred, typename A> void Serialize(Stream& os, const std::map<K, T, Pred, A>& m);
template<typename Stream, typename K, typename T, typename Pred, typename A> void Unserialize(Stream& is, std::map<K, T, Pred, A>& m);
template<typename Stream, typename K, typename Pred, typename A> void Serialize(Stream& os, const std::set<K, Pred, A>& m);
template<typename Stream, typename K, typename Pred, typename A> void Unserialize(Stream& is, std::set<K, Pred, A>& m);
template<typename Stream, typename T> void Serialize(Stream& os, const std::shared_ptr<const T>& p);
template<typename Stream, typename T> void Unserialize(Stream& os, std::shared_ptr<const T>& p);
template<typename Stream, typename T> void Serialize(Stream& os, const std::unique_ptr<const T>& p);
template<typename Stream, typename T> void Unserialize(Stream& os, std::unique_ptr<const T>& p);
template<typename Stream, typename T>
inline void Serialize(Stream& os, const T& a)
{
    a.Serialize(os);
}
template<typename Stream, typename T>
inline void Unserialize(Stream& is, T&& a)
{
    a.Unserialize(is);
}
struct DefaultFormatter
{
    template<typename Stream, typename T>
    static void Ser(Stream& s, const T& t) { Serialize(s, t); }
    template<typename Stream, typename T>
    static void Unser(Stream& s, T& t) { Unserialize(s, t); }
};
template<typename Stream, typename C>
void Serialize(Stream& os, const std::basic_string<C>& str)
{
    WriteCompactSize(os, str.size());
    if (!str.empty())
        os.write(MakeByteSpan(str));
}
template<typename Stream, typename C>
void Unserialize(Stream& is, std::basic_string<C>& str)
{
    unsigned int nSize = ReadCompactSize(is);
    str.resize(nSize);
    if (nSize != 0)
        is.read(MakeWritableByteSpan(str));
}
template<typename Stream, unsigned int N, typename T>
void Serialize_impl(Stream& os, const prevector<N, T>& v, const unsigned char&)
{
    WriteCompactSize(os, v.size());
    if (!v.empty())
        os.write(MakeByteSpan(v));
}
template<typename Stream, unsigned int N, typename T, typename V>
void Serialize_impl(Stream& os, const prevector<N, T>& v, const V&)
{
    Serialize(os, Using<VectorFormatter<DefaultFormatter>>(v));
}
template<typename Stream, unsigned int N, typename T>
inline void Serialize(Stream& os, const prevector<N, T>& v)
{
    Serialize_impl(os, v, T());
}
template<typename Stream, unsigned int N, typename T>
void Unserialize_impl(Stream& is, prevector<N, T>& v, const unsigned char&)
{
    v.clear();
    unsigned int nSize = ReadCompactSize(is);
    unsigned int i = 0;
    while (i < nSize)
    {
        unsigned int blk = std::min(nSize - i, (unsigned int)(1 + 4999999 / sizeof(T)));
        v.resize_uninitialized(i + blk);
        is.read(AsWritableBytes(Span{&v[i], blk}));
        i += blk;
    }
}
template<typename Stream, unsigned int N, typename T, typename V>
void Unserialize_impl(Stream& is, prevector<N, T>& v, const V&)
{
    Unserialize(is, Using<VectorFormatter<DefaultFormatter>>(v));
}
template<typename Stream, unsigned int N, typename T>
inline void Unserialize(Stream& is, prevector<N, T>& v)
{
    Unserialize_impl(is, v, T());
}
template<typename Stream, typename T, typename A>
void Serialize_impl(Stream& os, const std::vector<T, A>& v, const unsigned char&)
{
    WriteCompactSize(os, v.size());
    if (!v.empty())
        os.write(MakeByteSpan(v));
}
template<typename Stream, typename T, typename A>
void Serialize_impl(Stream& os, const std::vector<T, A>& v, const bool&)
{
    WriteCompactSize(os, v.size());
    for (bool elem : v) {
        ::Serialize(os, elem);
    }
}
template<typename Stream, typename T, typename A, typename V>
void Serialize_impl(Stream& os, const std::vector<T, A>& v, const V&)
{
    Serialize(os, Using<VectorFormatter<DefaultFormatter>>(v));
}
template<typename Stream, typename T, typename A>
inline void Serialize(Stream& os, const std::vector<T, A>& v)
{
    Serialize_impl(os, v, T());
}
template<typename Stream, typename T, typename A>
void Unserialize_impl(Stream& is, std::vector<T, A>& v, const unsigned char&)
{
    v.clear();
    unsigned int nSize = ReadCompactSize(is);
    unsigned int i = 0;
    while (i < nSize)
    {
        unsigned int blk = std::min(nSize - i, (unsigned int)(1 + 4999999 / sizeof(T)));
        v.resize(i + blk);
        is.read(AsWritableBytes(Span{&v[i], blk}));
        i += blk;
    }
}
template<typename Stream, typename T, typename A, typename V>
void Unserialize_impl(Stream& is, std::vector<T, A>& v, const V&)
{
    Unserialize(is, Using<VectorFormatter<DefaultFormatter>>(v));
}
template<typename Stream, typename T, typename A>
inline void Unserialize(Stream& is, std::vector<T, A>& v)
{
    Unserialize_impl(is, v, T());
}
template<typename Stream, typename K, typename T>
void Serialize(Stream& os, const std::pair<K, T>& item)
{
    Serialize(os, item.first);
    Serialize(os, item.second);
}
template<typename Stream, typename K, typename T>
void Unserialize(Stream& is, std::pair<K, T>& item)
{
    Unserialize(is, item.first);
    Unserialize(is, item.second);
}
template<typename Stream, typename K, typename T, typename Pred, typename A>
void Serialize(Stream& os, const std::map<K, T, Pred, A>& m)
{
    WriteCompactSize(os, m.size());
    for (const auto& entry : m)
        Serialize(os, entry);
}
template<typename Stream, typename K, typename T, typename Pred, typename A>
void Unserialize(Stream& is, std::map<K, T, Pred, A>& m)
{
    m.clear();
    unsigned int nSize = ReadCompactSize(is);
    typename std::map<K, T, Pred, A>::iterator mi = m.begin();
    for (unsigned int i = 0; i < nSize; i++)
    {
        std::pair<K, T> item;
        Unserialize(is, item);
        mi = m.insert(mi, item);
    }
}
template<typename Stream, typename K, typename Pred, typename A>
void Serialize(Stream& os, const std::set<K, Pred, A>& m)
{
    WriteCompactSize(os, m.size());
    for (typename std::set<K, Pred, A>::const_iterator it = m.begin(); it != m.end(); ++it)
        Serialize(os, (*it));
}
template<typename Stream, typename K, typename Pred, typename A>
void Unserialize(Stream& is, std::set<K, Pred, A>& m)
{
    m.clear();
    unsigned int nSize = ReadCompactSize(is);
    typename std::set<K, Pred, A>::iterator it = m.begin();
    for (unsigned int i = 0; i < nSize; i++)
    {
        K key;
        Unserialize(is, key);
        it = m.insert(it, key);
    }
}
template<typename Stream, typename T> void
Serialize(Stream& os, const std::unique_ptr<const T>& p)
{
    Serialize(os, *p);
}
template<typename Stream, typename T>
void Unserialize(Stream& is, std::unique_ptr<const T>& p)
{
    p.reset(new T(deserialize, is));
}
template<typename Stream, typename T> void
Serialize(Stream& os, const std::shared_ptr<const T>& p)
{
    Serialize(os, *p);
}
template<typename Stream, typename T>
void Unserialize(Stream& is, std::shared_ptr<const T>& p)
{
    p = std::make_shared<const T>(deserialize, is);
}
struct CSerActionSerialize
{
    constexpr bool ForRead() const { return false; }
};
struct CSerActionUnserialize
{
    constexpr bool ForRead() const { return true; }
};
class CSizeComputer
{
protected:
    size_t nSize;
    const int nVersion;
public:
    explicit CSizeComputer(int nVersionIn) : nSize(0), nVersion(nVersionIn) {}
    void write(Span<const std::byte> src)
    {
        this->nSize += src.size();
    }
    void seek(size_t _nSize)
    {
        this->nSize += _nSize;
    }
    template<typename T>
    CSizeComputer& operator<<(const T& obj)
    {
        ::Serialize(*this, obj);
        return (*this);
    }
    size_t size() const {
        return nSize;
    }
    int GetVersion() const { return nVersion; }
};
template<typename Stream>
void SerializeMany(Stream& s)
{
}
template<typename Stream, typename Arg, typename... Args>
void SerializeMany(Stream& s, const Arg& arg, const Args&... args)
{
    ::Serialize(s, arg);
    ::SerializeMany(s, args...);
}
template<typename Stream>
inline void UnserializeMany(Stream& s)
{
}
template<typename Stream, typename Arg, typename... Args>
inline void UnserializeMany(Stream& s, Arg&& arg, Args&&... args)
{
    ::Unserialize(s, arg);
    ::UnserializeMany(s, args...);
}
template<typename Stream, typename... Args>
inline void SerReadWriteMany(Stream& s, CSerActionSerialize ser_action, const Args&... args)
{
    ::SerializeMany(s, args...);
}
template<typename Stream, typename... Args>
inline void SerReadWriteMany(Stream& s, CSerActionUnserialize ser_action, Args&&... args)
{
    ::UnserializeMany(s, args...);
}
template<typename Stream, typename Type, typename Fn>
inline void SerRead(Stream& s, CSerActionSerialize ser_action, Type&&, Fn&&)
{
}
template<typename Stream, typename Type, typename Fn>
inline void SerRead(Stream& s, CSerActionUnserialize ser_action, Type&& obj, Fn&& fn)
{
    fn(s, std::forward<Type>(obj));
}
template<typename Stream, typename Type, typename Fn>
inline void SerWrite(Stream& s, CSerActionSerialize ser_action, Type&& obj, Fn&& fn)
{
    fn(s, std::forward<Type>(obj));
}
template<typename Stream, typename Type, typename Fn>
inline void SerWrite(Stream& s, CSerActionUnserialize ser_action, Type&&, Fn&&)
{
}
template<typename I>
inline void WriteVarInt(CSizeComputer &s, I n)
{
    s.seek(GetSizeOfVarInt<I>(n));
}
inline void WriteCompactSize(CSizeComputer &s, uint64_t nSize)
{
    s.seek(GetSizeOfCompactSize(nSize));
}
template <typename T>
size_t GetSerializeSize(const T& t, int nVersion = 0)
{
    return (CSizeComputer(nVersion) << t).size();
}
template <typename... T>
size_t GetSerializeSizeMany(int nVersion, const T&... t)
{
    CSizeComputer sc(nVersion);
    SerializeMany(sc, t...);
    return sc.size();
}
#endif
