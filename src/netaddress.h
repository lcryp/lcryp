#ifndef LCRYP_NETADDRESS_H
#define LCRYP_NETADDRESS_H
#if defined(HAVE_CONFIG_H)
#include <config/lcryp-config.h>
#endif
#include <compat/compat.h>
#include <crypto/siphash.h>
#include <prevector.h>
#include <random.h>
#include <serialize.h>
#include <tinyformat.h>
#include <util/strencodings.h>
#include <util/string.h>
#include <array>
#include <cstdint>
#include <ios>
#include <string>
#include <vector>
static constexpr int ADDRV2_FORMAT = 0x20000000;
enum Network {
    NET_UNROUTABLE = 0,
    NET_IPV4,
    NET_IPV6,
    NET_ONION,
    NET_I2P,
    NET_CJDNS,
    NET_INTERNAL,
    NET_MAX,
};
static const std::array<uint8_t, 12> IPV4_IN_IPV6_PREFIX{
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF};
static const std::array<uint8_t, 6> TORV2_IN_IPV6_PREFIX{
    0xFD, 0x87, 0xD8, 0x7E, 0xEB, 0x43};
static const std::array<uint8_t, 6> INTERNAL_IN_IPV6_PREFIX{
    0xFD, 0x6B, 0x88, 0xC0, 0x87, 0x24
};
static constexpr size_t ADDR_IPV4_SIZE = 4;
static constexpr size_t ADDR_IPV6_SIZE = 16;
static constexpr size_t ADDR_TORV3_SIZE = 32;
static constexpr size_t ADDR_I2P_SIZE = 32;
static constexpr size_t ADDR_CJDNS_SIZE = 16;
static constexpr size_t ADDR_INTERNAL_SIZE = 10;
static constexpr uint16_t I2P_SAM31_PORT{0};
class CNetAddr
{
protected:
    prevector<ADDR_IPV6_SIZE, uint8_t> m_addr{ADDR_IPV6_SIZE, 0x0};
    Network m_net{NET_IPV6};
    uint32_t m_scope_id{0};
public:
    CNetAddr();
    explicit CNetAddr(const struct in_addr& ipv4Addr);
    void SetIP(const CNetAddr& ip);
    void SetLegacyIPv6(Span<const uint8_t> ipv6);
    bool SetInternal(const std::string& name);
    bool SetSpecial(const std::string& addr);
    bool IsBindAny() const;
    bool IsIPv4() const;
    bool IsIPv6() const;
    bool IsRFC1918() const;
    bool IsRFC2544() const;
    bool IsRFC6598() const;
    bool IsRFC5737() const;
    bool IsRFC3849() const;
    bool IsRFC3927() const;
    bool IsRFC3964() const;
    bool IsRFC4193() const;
    bool IsRFC4380() const;
    bool IsRFC4843() const;
    bool IsRFC7343() const;
    bool IsRFC4862() const;
    bool IsRFC6052() const;
    bool IsRFC6145() const;
    bool IsHeNet() const;
    bool IsTor() const;
    bool IsI2P() const;
    bool IsCJDNS() const;
    bool IsLocal() const;
    bool IsRoutable() const;
    bool IsInternal() const;
    bool IsValid() const;
    bool IsAddrV1Compatible() const;
    enum Network GetNetwork() const;
    std::string ToString() const;
    std::string ToStringIP() const;
    bool GetInAddr(struct in_addr* pipv4Addr) const;
    Network GetNetClass() const;
    uint32_t GetLinkedIPv4() const;
    bool HasLinkedIPv4() const;
    std::vector<unsigned char> GetAddrBytes() const;
    int GetReachabilityFrom(const CNetAddr* paddrPartner = nullptr) const;
    explicit CNetAddr(const struct in6_addr& pipv6Addr, const uint32_t scope = 0);
    bool GetIn6Addr(struct in6_addr* pipv6Addr) const;
    friend bool operator==(const CNetAddr& a, const CNetAddr& b);
    friend bool operator!=(const CNetAddr& a, const CNetAddr& b) { return !(a == b); }
    friend bool operator<(const CNetAddr& a, const CNetAddr& b);
    bool IsRelayable() const
    {
        return IsIPv4() || IsIPv6() || IsTor() || IsI2P() || IsCJDNS();
    }
    template <typename Stream>
    void Serialize(Stream& s) const
    {
        if (s.GetVersion() & ADDRV2_FORMAT) {
            SerializeV2Stream(s);
        } else {
            SerializeV1Stream(s);
        }
    }
    template <typename Stream>
    void Unserialize(Stream& s)
    {
        if (s.GetVersion() & ADDRV2_FORMAT) {
            UnserializeV2Stream(s);
        } else {
            UnserializeV1Stream(s);
        }
    }
    friend class CSubNet;
private:
    bool SetTor(const std::string& addr);
    bool SetI2P(const std::string& addr);
    enum BIP155Network : uint8_t {
        IPV4 = 1,
        IPV6 = 2,
        TORV2 = 3,
        TORV3 = 4,
        I2P = 5,
        CJDNS = 6,
    };
    static constexpr size_t V1_SERIALIZATION_SIZE = ADDR_IPV6_SIZE;
    static constexpr size_t MAX_ADDRV2_SIZE = 512;
    BIP155Network GetBIP155Network() const;
    bool SetNetFromBIP155Network(uint8_t possible_bip155_net, size_t address_size);
    void SerializeV1Array(uint8_t (&arr)[V1_SERIALIZATION_SIZE]) const
    {
        size_t prefix_size;
        switch (m_net) {
        case NET_IPV6:
            assert(m_addr.size() == sizeof(arr));
            memcpy(arr, m_addr.data(), m_addr.size());
            return;
        case NET_IPV4:
            prefix_size = sizeof(IPV4_IN_IPV6_PREFIX);
            assert(prefix_size + m_addr.size() == sizeof(arr));
            memcpy(arr, IPV4_IN_IPV6_PREFIX.data(), prefix_size);
            memcpy(arr + prefix_size, m_addr.data(), m_addr.size());
            return;
        case NET_INTERNAL:
            prefix_size = sizeof(INTERNAL_IN_IPV6_PREFIX);
            assert(prefix_size + m_addr.size() == sizeof(arr));
            memcpy(arr, INTERNAL_IN_IPV6_PREFIX.data(), prefix_size);
            memcpy(arr + prefix_size, m_addr.data(), m_addr.size());
            return;
        case NET_ONION:
        case NET_I2P:
        case NET_CJDNS:
            break;
        case NET_UNROUTABLE:
        case NET_MAX:
            assert(false);
        }
        memset(arr, 0x0, V1_SERIALIZATION_SIZE);
    }
    template <typename Stream>
    void SerializeV1Stream(Stream& s) const
    {
        uint8_t serialized[V1_SERIALIZATION_SIZE];
        SerializeV1Array(serialized);
        s << serialized;
    }
    template <typename Stream>
    void SerializeV2Stream(Stream& s) const
    {
        if (IsInternal()) {
            s << static_cast<uint8_t>(BIP155Network::IPV6);
            s << COMPACTSIZE(ADDR_IPV6_SIZE);
            SerializeV1Stream(s);
            return;
        }
        s << static_cast<uint8_t>(GetBIP155Network());
        s << m_addr;
    }
    void UnserializeV1Array(uint8_t (&arr)[V1_SERIALIZATION_SIZE])
    {
        SetLegacyIPv6(arr);
    }
    template <typename Stream>
    void UnserializeV1Stream(Stream& s)
    {
        uint8_t serialized[V1_SERIALIZATION_SIZE];
        s >> serialized;
        UnserializeV1Array(serialized);
    }
    template <typename Stream>
    void UnserializeV2Stream(Stream& s)
    {
        uint8_t bip155_net;
        s >> bip155_net;
        size_t address_size;
        s >> COMPACTSIZE(address_size);
        if (address_size > MAX_ADDRV2_SIZE) {
            throw std::ios_base::failure(strprintf(
                "Address too long: %u > %u", address_size, MAX_ADDRV2_SIZE));
        }
        m_scope_id = 0;
        if (SetNetFromBIP155Network(bip155_net, address_size)) {
            m_addr.resize(address_size);
            s >> Span{m_addr};
            if (m_net != NET_IPV6) {
                return;
            }
            if (HasPrefix(m_addr, INTERNAL_IN_IPV6_PREFIX)) {
                m_net = NET_INTERNAL;
                memmove(m_addr.data(), m_addr.data() + INTERNAL_IN_IPV6_PREFIX.size(),
                        ADDR_INTERNAL_SIZE);
                m_addr.resize(ADDR_INTERNAL_SIZE);
                return;
            }
            if (!HasPrefix(m_addr, IPV4_IN_IPV6_PREFIX) &&
                !HasPrefix(m_addr, TORV2_IN_IPV6_PREFIX)) {
                return;
            }
        } else {
            s.ignore(address_size);
        }
        m_net = NET_IPV6;
        m_addr.assign(ADDR_IPV6_SIZE, 0x0);
    }
};
class CSubNet
{
protected:
    CNetAddr network;
    uint8_t netmask[16];
    bool valid;
    bool SanityCheck() const;
public:
    CSubNet();
    CSubNet(const CNetAddr& addr, uint8_t mask);
    CSubNet(const CNetAddr& addr, const CNetAddr& mask);
    explicit CSubNet(const CNetAddr& addr);
    bool Match(const CNetAddr& addr) const;
    std::string ToString() const;
    bool IsValid() const;
    friend bool operator==(const CSubNet& a, const CSubNet& b);
    friend bool operator!=(const CSubNet& a, const CSubNet& b) { return !(a == b); }
    friend bool operator<(const CSubNet& a, const CSubNet& b);
};
class CService : public CNetAddr
{
protected:
    uint16_t port;
public:
    CService();
    CService(const CNetAddr& ip, uint16_t port);
    CService(const struct in_addr& ipv4Addr, uint16_t port);
    explicit CService(const struct sockaddr_in& addr);
    uint16_t GetPort() const;
    bool GetSockAddr(struct sockaddr* paddr, socklen_t* addrlen) const;
    bool SetSockAddr(const struct sockaddr* paddr);
    friend bool operator==(const CService& a, const CService& b);
    friend bool operator!=(const CService& a, const CService& b) { return !(a == b); }
    friend bool operator<(const CService& a, const CService& b);
    std::vector<unsigned char> GetKey() const;
    std::string ToString() const;
    std::string ToStringPort() const;
    std::string ToStringIPPort() const;
    CService(const struct in6_addr& ipv6Addr, uint16_t port);
    explicit CService(const struct sockaddr_in6& addr);
    SERIALIZE_METHODS(CService, obj)
    {
        READWRITEAS(CNetAddr, obj);
        READWRITE(Using<BigEndianFormatter<2>>(obj.port));
    }
    friend class CServiceHash;
    friend CService MaybeFlipIPv6toCJDNS(const CService& service);
};
class CServiceHash
{
public:
    CServiceHash()
        : m_salt_k0{GetRand<uint64_t>()},
          m_salt_k1{GetRand<uint64_t>()}
    {
    }
    CServiceHash(uint64_t salt_k0, uint64_t salt_k1) : m_salt_k0{salt_k0}, m_salt_k1{salt_k1} {}
    size_t operator()(const CService& a) const noexcept
    {
        CSipHasher hasher(m_salt_k0, m_salt_k1);
        hasher.Write(a.m_net);
        hasher.Write(a.port);
        hasher.Write(a.m_addr.data(), a.m_addr.size());
        return static_cast<size_t>(hasher.Finalize());
    }
private:
    const uint64_t m_salt_k0;
    const uint64_t m_salt_k1;
};
#endif
