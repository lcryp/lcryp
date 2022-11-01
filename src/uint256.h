#ifndef LCRYP_UINT256_H
#define LCRYP_UINT256_H
#include <crypto/common.h>
#include <span.h>
#include <assert.h>
#include <cstring>
#include <stdint.h>
#include <string>
#include <vector>
template<unsigned int BITS>
class base_blob
{
protected:
    static constexpr int WIDTH = BITS / 8;
    uint8_t m_data[WIDTH];
public:
    constexpr base_blob() : m_data() {}
    constexpr explicit base_blob(uint8_t v) : m_data{v} {}
    explicit base_blob(const std::vector<unsigned char>& vch);
    bool IsNull() const
    {
        for (int i = 0; i < WIDTH; i++)
            if (m_data[i] != 0)
                return false;
        return true;
    }
    void SetNull()
    {
        memset(m_data, 0, sizeof(m_data));
    }
    inline int Compare(const base_blob& other) const { return memcmp(m_data, other.m_data, sizeof(m_data)); }
    friend inline bool operator==(const base_blob& a, const base_blob& b) { return a.Compare(b) == 0; }
    friend inline bool operator!=(const base_blob& a, const base_blob& b) { return a.Compare(b) != 0; }
    friend inline bool operator<(const base_blob& a, const base_blob& b) { return a.Compare(b) < 0; }
    std::string GetHex() const;
    void SetHex(const char* psz);
    void SetHex(const std::string& str);
    std::string ToString() const;
    const unsigned char* data() const { return m_data; }
    unsigned char* data() { return m_data; }
    unsigned char* begin()
    {
        return &m_data[0];
    }
    unsigned char* end()
    {
        return &m_data[WIDTH];
    }
    const unsigned char* begin() const
    {
        return &m_data[0];
    }
    const unsigned char* end() const
    {
        return &m_data[WIDTH];
    }
    static constexpr unsigned int size()
    {
        return sizeof(m_data);
    }
    uint64_t GetUint64(int pos) const
    {
        return ReadLE64(m_data + pos * 8);
    }
    template<typename Stream>
    void Serialize(Stream& s) const
    {
        s.write(MakeByteSpan(m_data));
    }
    template<typename Stream>
    void Unserialize(Stream& s)
    {
        s.read(MakeWritableByteSpan(m_data));
    }
};
class uint160 : public base_blob<160> {
public:
    constexpr uint160() {}
    explicit uint160(const std::vector<unsigned char>& vch) : base_blob<160>(vch) {}
};
class uint256 : public base_blob<256> {
public:
    constexpr uint256() {}
    constexpr explicit uint256(uint8_t v) : base_blob<256>(v) {}
    explicit uint256(const std::vector<unsigned char>& vch) : base_blob<256>(vch) {}
    static const uint256 ZERO;
    static const uint256 ONE;
};
inline uint256 uint256S(const char *str)
{
    uint256 rv;
    rv.SetHex(str);
    return rv;
}
inline uint256 uint256S(const std::string& str)
{
    uint256 rv;
    rv.SetHex(str);
    return rv;
}
#endif
