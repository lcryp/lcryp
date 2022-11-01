#include <netaddress.h>
#include <crypto/common.h>
#include <crypto/sha3.h>
#include <hash.h>
#include <prevector.h>
#include <tinyformat.h>
#include <util/strencodings.h>
#include <util/string.h>
#include <algorithm>
#include <array>
#include <cstdint>
#include <ios>
#include <iterator>
#include <tuple>
CNetAddr::BIP155Network CNetAddr::GetBIP155Network() const
{
    switch (m_net) {
    case NET_IPV4:
        return BIP155Network::IPV4;
    case NET_IPV6:
        return BIP155Network::IPV6;
    case NET_ONION:
        return BIP155Network::TORV3;
    case NET_I2P:
        return BIP155Network::I2P;
    case NET_CJDNS:
        return BIP155Network::CJDNS;
    case NET_INTERNAL:
    case NET_UNROUTABLE:
    case NET_MAX:
        assert(false);
    }
    assert(false);
}
bool CNetAddr::SetNetFromBIP155Network(uint8_t possible_bip155_net, size_t address_size)
{
    switch (possible_bip155_net) {
    case BIP155Network::IPV4:
        if (address_size == ADDR_IPV4_SIZE) {
            m_net = NET_IPV4;
            return true;
        }
        throw std::ios_base::failure(
            strprintf("BIP155 IPv4 address with length %u (should be %u)", address_size,
                      ADDR_IPV4_SIZE));
    case BIP155Network::IPV6:
        if (address_size == ADDR_IPV6_SIZE) {
            m_net = NET_IPV6;
            return true;
        }
        throw std::ios_base::failure(
            strprintf("BIP155 IPv6 address with length %u (should be %u)", address_size,
                      ADDR_IPV6_SIZE));
    case BIP155Network::TORV3:
        if (address_size == ADDR_TORV3_SIZE) {
            m_net = NET_ONION;
            return true;
        }
        throw std::ios_base::failure(
            strprintf("BIP155 TORv3 address with length %u (should be %u)", address_size,
                      ADDR_TORV3_SIZE));
    case BIP155Network::I2P:
        if (address_size == ADDR_I2P_SIZE) {
            m_net = NET_I2P;
            return true;
        }
        throw std::ios_base::failure(
            strprintf("BIP155 I2P address with length %u (should be %u)", address_size,
                      ADDR_I2P_SIZE));
    case BIP155Network::CJDNS:
        if (address_size == ADDR_CJDNS_SIZE) {
            m_net = NET_CJDNS;
            return true;
        }
        throw std::ios_base::failure(
            strprintf("BIP155 CJDNS address with length %u (should be %u)", address_size,
                      ADDR_CJDNS_SIZE));
    }
    return false;
}
CNetAddr::CNetAddr() = default;
void CNetAddr::SetIP(const CNetAddr& ipIn)
{
    switch (ipIn.m_net) {
    case NET_IPV4:
        assert(ipIn.m_addr.size() == ADDR_IPV4_SIZE);
        break;
    case NET_IPV6:
        assert(ipIn.m_addr.size() == ADDR_IPV6_SIZE);
        break;
    case NET_ONION:
        assert(ipIn.m_addr.size() == ADDR_TORV3_SIZE);
        break;
    case NET_I2P:
        assert(ipIn.m_addr.size() == ADDR_I2P_SIZE);
        break;
    case NET_CJDNS:
        assert(ipIn.m_addr.size() == ADDR_CJDNS_SIZE);
        break;
    case NET_INTERNAL:
        assert(ipIn.m_addr.size() == ADDR_INTERNAL_SIZE);
        break;
    case NET_UNROUTABLE:
    case NET_MAX:
        assert(false);
    }
    m_net = ipIn.m_net;
    m_addr = ipIn.m_addr;
}
void CNetAddr::SetLegacyIPv6(Span<const uint8_t> ipv6)
{
    assert(ipv6.size() == ADDR_IPV6_SIZE);
    size_t skip{0};
    if (HasPrefix(ipv6, IPV4_IN_IPV6_PREFIX)) {
        m_net = NET_IPV4;
        skip = sizeof(IPV4_IN_IPV6_PREFIX);
    } else if (HasPrefix(ipv6, TORV2_IN_IPV6_PREFIX)) {
        m_net = NET_IPV6;
        m_addr.assign(ADDR_IPV6_SIZE, 0x0);
        return;
    } else if (HasPrefix(ipv6, INTERNAL_IN_IPV6_PREFIX)) {
        m_net = NET_INTERNAL;
        skip = sizeof(INTERNAL_IN_IPV6_PREFIX);
    } else {
        m_net = NET_IPV6;
    }
    m_addr.assign(ipv6.begin() + skip, ipv6.end());
}
bool CNetAddr::SetInternal(const std::string &name)
{
    if (name.empty()) {
        return false;
    }
    m_net = NET_INTERNAL;
    unsigned char hash[32] = {};
    CSHA256().Write((const unsigned char*)name.data(), name.size()).Finalize(hash);
    m_addr.assign(hash, hash + ADDR_INTERNAL_SIZE);
    return true;
}
namespace torv3 {
static constexpr size_t CHECKSUM_LEN = 2;
static const unsigned char VERSION[] = {3};
static constexpr size_t TOTAL_LEN = ADDR_TORV3_SIZE + CHECKSUM_LEN + sizeof(VERSION);
static void Checksum(Span<const uint8_t> addr_pubkey, uint8_t (&checksum)[CHECKSUM_LEN])
{
    static const unsigned char prefix[] = ".onion checksum";
    static constexpr size_t prefix_len = 15;
    SHA3_256 hasher;
    hasher.Write(Span{prefix}.first(prefix_len));
    hasher.Write(addr_pubkey);
    hasher.Write(VERSION);
    uint8_t checksum_full[SHA3_256::OUTPUT_SIZE];
    hasher.Finalize(checksum_full);
    memcpy(checksum, checksum_full, sizeof(checksum));
}
};
bool CNetAddr::SetSpecial(const std::string& addr)
{
    if (!ContainsNoNUL(addr)) {
        return false;
    }
    if (SetTor(addr)) {
        return true;
    }
    if (SetI2P(addr)) {
        return true;
    }
    return false;
}
bool CNetAddr::SetTor(const std::string& addr)
{
    static const char* suffix{".onion"};
    static constexpr size_t suffix_len{6};
    if (addr.size() <= suffix_len || addr.substr(addr.size() - suffix_len) != suffix) {
        return false;
    }
    auto input = DecodeBase32(std::string_view{addr}.substr(0, addr.size() - suffix_len));
    if (!input) {
        return false;
    }
    if (input->size() == torv3::TOTAL_LEN) {
        Span<const uint8_t> input_pubkey{input->data(), ADDR_TORV3_SIZE};
        Span<const uint8_t> input_checksum{input->data() + ADDR_TORV3_SIZE, torv3::CHECKSUM_LEN};
        Span<const uint8_t> input_version{input->data() + ADDR_TORV3_SIZE + torv3::CHECKSUM_LEN, sizeof(torv3::VERSION)};
        if (input_version != torv3::VERSION) {
            return false;
        }
        uint8_t calculated_checksum[torv3::CHECKSUM_LEN];
        torv3::Checksum(input_pubkey, calculated_checksum);
        if (input_checksum != calculated_checksum) {
            return false;
        }
        m_net = NET_ONION;
        m_addr.assign(input_pubkey.begin(), input_pubkey.end());
        return true;
    }
    return false;
}
bool CNetAddr::SetI2P(const std::string& addr)
{
    static constexpr size_t b32_len{52};
    static const char* suffix{".b32.i2p"};
    static constexpr size_t suffix_len{8};
    if (addr.size() != b32_len + suffix_len || ToLower(addr.substr(b32_len)) != suffix) {
        return false;
    }
    const std::string b32_padded = addr.substr(0, b32_len) + "====";
    auto address_bytes = DecodeBase32(b32_padded);
    if (!address_bytes || address_bytes->size() != ADDR_I2P_SIZE) {
        return false;
    }
    m_net = NET_I2P;
    m_addr.assign(address_bytes->begin(), address_bytes->end());
    return true;
}
CNetAddr::CNetAddr(const struct in_addr& ipv4Addr)
{
    m_net = NET_IPV4;
    const uint8_t* ptr = reinterpret_cast<const uint8_t*>(&ipv4Addr);
    m_addr.assign(ptr, ptr + ADDR_IPV4_SIZE);
}
CNetAddr::CNetAddr(const struct in6_addr& ipv6Addr, const uint32_t scope)
{
    SetLegacyIPv6({reinterpret_cast<const uint8_t*>(&ipv6Addr), sizeof(ipv6Addr)});
    m_scope_id = scope;
}
bool CNetAddr::IsBindAny() const
{
    if (!IsIPv4() && !IsIPv6()) {
        return false;
    }
    return std::all_of(m_addr.begin(), m_addr.end(), [](uint8_t b) { return b == 0; });
}
bool CNetAddr::IsIPv4() const { return m_net == NET_IPV4; }
bool CNetAddr::IsIPv6() const { return m_net == NET_IPV6; }
bool CNetAddr::IsRFC1918() const
{
    return IsIPv4() && (
        m_addr[0] == 10 ||
        (m_addr[0] == 192 && m_addr[1] == 168) ||
        (m_addr[0] == 172 && m_addr[1] >= 16 && m_addr[1] <= 31));
}
bool CNetAddr::IsRFC2544() const
{
    return IsIPv4() && m_addr[0] == 198 && (m_addr[1] == 18 || m_addr[1] == 19);
}
bool CNetAddr::IsRFC3927() const
{
    return IsIPv4() && HasPrefix(m_addr, std::array<uint8_t, 2>{169, 254});
}
bool CNetAddr::IsRFC6598() const
{
    return IsIPv4() && m_addr[0] == 100 && m_addr[1] >= 64 && m_addr[1] <= 127;
}
bool CNetAddr::IsRFC5737() const
{
    return IsIPv4() && (HasPrefix(m_addr, std::array<uint8_t, 3>{192, 0, 2}) ||
                        HasPrefix(m_addr, std::array<uint8_t, 3>{198, 51, 100}) ||
                        HasPrefix(m_addr, std::array<uint8_t, 3>{203, 0, 113}));
}
bool CNetAddr::IsRFC3849() const
{
    return IsIPv6() && HasPrefix(m_addr, std::array<uint8_t, 4>{0x20, 0x01, 0x0D, 0xB8});
}
bool CNetAddr::IsRFC3964() const
{
    return IsIPv6() && HasPrefix(m_addr, std::array<uint8_t, 2>{0x20, 0x02});
}
bool CNetAddr::IsRFC6052() const
{
    return IsIPv6() &&
           HasPrefix(m_addr, std::array<uint8_t, 12>{0x00, 0x64, 0xFF, 0x9B, 0x00, 0x00,
                                                     0x00, 0x00, 0x00, 0x00, 0x00, 0x00});
}
bool CNetAddr::IsRFC4380() const
{
    return IsIPv6() && HasPrefix(m_addr, std::array<uint8_t, 4>{0x20, 0x01, 0x00, 0x00});
}
bool CNetAddr::IsRFC4862() const
{
    return IsIPv6() && HasPrefix(m_addr, std::array<uint8_t, 8>{0xFE, 0x80, 0x00, 0x00,
                                                                0x00, 0x00, 0x00, 0x00});
}
bool CNetAddr::IsRFC4193() const
{
    return IsIPv6() && (m_addr[0] & 0xFE) == 0xFC;
}
bool CNetAddr::IsRFC6145() const
{
    return IsIPv6() &&
           HasPrefix(m_addr, std::array<uint8_t, 12>{0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                     0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00});
}
bool CNetAddr::IsRFC4843() const
{
    return IsIPv6() && HasPrefix(m_addr, std::array<uint8_t, 3>{0x20, 0x01, 0x00}) &&
           (m_addr[3] & 0xF0) == 0x10;
}
bool CNetAddr::IsRFC7343() const
{
    return IsIPv6() && HasPrefix(m_addr, std::array<uint8_t, 3>{0x20, 0x01, 0x00}) &&
           (m_addr[3] & 0xF0) == 0x20;
}
bool CNetAddr::IsHeNet() const
{
    return IsIPv6() && HasPrefix(m_addr, std::array<uint8_t, 4>{0x20, 0x01, 0x04, 0x70});
}
bool CNetAddr::IsTor() const { return m_net == NET_ONION; }
bool CNetAddr::IsI2P() const { return m_net == NET_I2P; }
bool CNetAddr::IsCJDNS() const { return m_net == NET_CJDNS; }
bool CNetAddr::IsLocal() const
{
    if (IsIPv4() && (m_addr[0] == 127 || m_addr[0] == 0)) {
        return true;
    }
    static const unsigned char pchLocal[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1};
    if (IsIPv6() && memcmp(m_addr.data(), pchLocal, sizeof(pchLocal)) == 0) {
        return true;
    }
    return false;
}
bool CNetAddr::IsValid() const
{
    unsigned char ipNone6[16] = {};
    if (IsIPv6() && memcmp(m_addr.data(), ipNone6, sizeof(ipNone6)) == 0) {
        return false;
    }
    if (IsCJDNS() && (m_addr[0] != 0xFC)) {
        return false;
    }
    if (IsRFC3849())
        return false;
    if (IsInternal())
        return false;
    if (IsIPv4()) {
        const uint32_t addr = ReadBE32(m_addr.data());
        if (addr == INADDR_ANY || addr == INADDR_NONE) {
            return false;
        }
    }
    return true;
}
bool CNetAddr::IsRoutable() const
{
    return IsValid() && !(IsRFC1918() || IsRFC2544() || IsRFC3927() || IsRFC4862() || IsRFC6598() || IsRFC5737() || IsRFC4193() || IsRFC4843() || IsRFC7343() || IsLocal() || IsInternal());
}
bool CNetAddr::IsInternal() const
{
   return m_net == NET_INTERNAL;
}
bool CNetAddr::IsAddrV1Compatible() const
{
    switch (m_net) {
    case NET_IPV4:
    case NET_IPV6:
    case NET_INTERNAL:
        return true;
    case NET_ONION:
    case NET_I2P:
    case NET_CJDNS:
        return false;
    case NET_UNROUTABLE:
    case NET_MAX:
        assert(false);
    }
    assert(false);
}
enum Network CNetAddr::GetNetwork() const
{
    if (IsInternal())
        return NET_INTERNAL;
    if (!IsRoutable())
        return NET_UNROUTABLE;
    return m_net;
}
static std::string IPv4ToString(Span<const uint8_t> a)
{
    return strprintf("%u.%u.%u.%u", a[0], a[1], a[2], a[3]);
}
static std::string IPv6ToString(Span<const uint8_t> a, uint32_t scope_id)
{
    assert(a.size() == ADDR_IPV6_SIZE);
    const std::array groups{
        ReadBE16(&a[0]),
        ReadBE16(&a[2]),
        ReadBE16(&a[4]),
        ReadBE16(&a[6]),
        ReadBE16(&a[8]),
        ReadBE16(&a[10]),
        ReadBE16(&a[12]),
        ReadBE16(&a[14]),
    };
    struct ZeroSpan {
        size_t start_index{0};
        size_t len{0};
    };
    ZeroSpan longest, current;
    for (size_t i{0}; i < groups.size(); ++i) {
        if (groups[i] != 0) {
            current = {i + 1, 0};
            continue;
        }
        current.len += 1;
        if (current.len > longest.len) {
            longest = current;
        }
    }
    std::string r;
    r.reserve(39);
    for (size_t i{0}; i < groups.size(); ++i) {
        if (longest.len >= 2 && i >= longest.start_index && i < longest.start_index + longest.len) {
            if (i == longest.start_index) {
                r += "::";
            }
            continue;
        }
        r += strprintf("%s%x", ((!r.empty() && r.back() != ':') ? ":" : ""), groups[i]);
    }
    if (scope_id != 0) {
        r += strprintf("%%%u", scope_id);
    }
    return r;
}
static std::string OnionToString(Span<const uint8_t> addr)
{
    uint8_t checksum[torv3::CHECKSUM_LEN];
    torv3::Checksum(addr, checksum);
    prevector<torv3::TOTAL_LEN, uint8_t> address{addr.begin(), addr.end()};
    address.insert(address.end(), checksum, checksum + torv3::CHECKSUM_LEN);
    address.insert(address.end(), torv3::VERSION, torv3::VERSION + sizeof(torv3::VERSION));
    return EncodeBase32(address) + ".onion";
}
std::string CNetAddr::ToStringIP() const
{
    switch (m_net) {
    case NET_IPV4:
        return IPv4ToString(m_addr);
    case NET_IPV6:
        return IPv6ToString(m_addr, m_scope_id);
    case NET_ONION:
        return OnionToString(m_addr);
    case NET_I2P:
        return EncodeBase32(m_addr, false  ) + ".b32.i2p";
    case NET_CJDNS:
        return IPv6ToString(m_addr, 0);
    case NET_INTERNAL:
        return EncodeBase32(m_addr) + ".internal";
    case NET_UNROUTABLE:
    case NET_MAX:
        assert(false);
    }
    assert(false);
}
std::string CNetAddr::ToString() const
{
    return ToStringIP();
}
bool operator==(const CNetAddr& a, const CNetAddr& b)
{
    return a.m_net == b.m_net && a.m_addr == b.m_addr;
}
bool operator<(const CNetAddr& a, const CNetAddr& b)
{
    return std::tie(a.m_net, a.m_addr) < std::tie(b.m_net, b.m_addr);
}
bool CNetAddr::GetInAddr(struct in_addr* pipv4Addr) const
{
    if (!IsIPv4())
        return false;
    assert(sizeof(*pipv4Addr) == m_addr.size());
    memcpy(pipv4Addr, m_addr.data(), m_addr.size());
    return true;
}
bool CNetAddr::GetIn6Addr(struct in6_addr* pipv6Addr) const
{
    if (!IsIPv6() && !IsCJDNS()) {
        return false;
    }
    assert(sizeof(*pipv6Addr) == m_addr.size());
    memcpy(pipv6Addr, m_addr.data(), m_addr.size());
    return true;
}
bool CNetAddr::HasLinkedIPv4() const
{
    return IsRoutable() && (IsIPv4() || IsRFC6145() || IsRFC6052() || IsRFC3964() || IsRFC4380());
}
uint32_t CNetAddr::GetLinkedIPv4() const
{
    if (IsIPv4()) {
        return ReadBE32(m_addr.data());
    } else if (IsRFC6052() || IsRFC6145()) {
        return ReadBE32(Span{m_addr}.last(ADDR_IPV4_SIZE).data());
    } else if (IsRFC3964()) {
        return ReadBE32(Span{m_addr}.subspan(2, ADDR_IPV4_SIZE).data());
    } else if (IsRFC4380()) {
        return ~ReadBE32(Span{m_addr}.last(ADDR_IPV4_SIZE).data());
    }
    assert(false);
}
Network CNetAddr::GetNetClass() const
{
    if (IsInternal()) {
        return NET_INTERNAL;
    }
    if (!IsRoutable()) {
        return NET_UNROUTABLE;
    }
    if (HasLinkedIPv4()) {
        return NET_IPV4;
    }
    return m_net;
}
std::vector<unsigned char> CNetAddr::GetAddrBytes() const
{
    if (IsAddrV1Compatible()) {
        uint8_t serialized[V1_SERIALIZATION_SIZE];
        SerializeV1Array(serialized);
        return {std::begin(serialized), std::end(serialized)};
    }
    return std::vector<unsigned char>(m_addr.begin(), m_addr.end());
}
static const int NET_UNKNOWN = NET_MAX + 0;
static const int NET_TEREDO  = NET_MAX + 1;
int static GetExtNetwork(const CNetAddr *addr)
{
    if (addr == nullptr)
        return NET_UNKNOWN;
    if (addr->IsRFC4380())
        return NET_TEREDO;
    return addr->GetNetwork();
}
int CNetAddr::GetReachabilityFrom(const CNetAddr *paddrPartner) const
{
    enum Reachability {
        REACH_UNREACHABLE,
        REACH_DEFAULT,
        REACH_TEREDO,
        REACH_IPV6_WEAK,
        REACH_IPV4,
        REACH_IPV6_STRONG,
        REACH_PRIVATE
    };
    if (!IsRoutable() || IsInternal())
        return REACH_UNREACHABLE;
    int ourNet = GetExtNetwork(this);
    int theirNet = GetExtNetwork(paddrPartner);
    bool fTunnel = IsRFC3964() || IsRFC6052() || IsRFC6145();
    switch(theirNet) {
    case NET_IPV4:
        switch(ourNet) {
        default:       return REACH_DEFAULT;
        case NET_IPV4: return REACH_IPV4;
        }
    case NET_IPV6:
        switch(ourNet) {
        default:         return REACH_DEFAULT;
        case NET_TEREDO: return REACH_TEREDO;
        case NET_IPV4:   return REACH_IPV4;
        case NET_IPV6:   return fTunnel ? REACH_IPV6_WEAK : REACH_IPV6_STRONG;
        }
    case NET_ONION:
        switch(ourNet) {
        default:         return REACH_DEFAULT;
        case NET_IPV4:   return REACH_IPV4;
        case NET_ONION:    return REACH_PRIVATE;
        }
    case NET_I2P:
        switch (ourNet) {
        case NET_I2P: return REACH_PRIVATE;
        default: return REACH_DEFAULT;
        }
    case NET_CJDNS:
        switch (ourNet) {
        case NET_CJDNS: return REACH_PRIVATE;
        default: return REACH_DEFAULT;
        }
    case NET_TEREDO:
        switch(ourNet) {
        default:          return REACH_DEFAULT;
        case NET_TEREDO:  return REACH_TEREDO;
        case NET_IPV6:    return REACH_IPV6_WEAK;
        case NET_IPV4:    return REACH_IPV4;
        }
    case NET_UNKNOWN:
    case NET_UNROUTABLE:
    default:
        switch(ourNet) {
        default:          return REACH_DEFAULT;
        case NET_TEREDO:  return REACH_TEREDO;
        case NET_IPV6:    return REACH_IPV6_WEAK;
        case NET_IPV4:    return REACH_IPV4;
        case NET_ONION:     return REACH_PRIVATE;
        }
    }
}
CService::CService() : port(0)
{
}
CService::CService(const CNetAddr& cip, uint16_t portIn) : CNetAddr(cip), port(portIn)
{
}
CService::CService(const struct in_addr& ipv4Addr, uint16_t portIn) : CNetAddr(ipv4Addr), port(portIn)
{
}
CService::CService(const struct in6_addr& ipv6Addr, uint16_t portIn) : CNetAddr(ipv6Addr), port(portIn)
{
}
CService::CService(const struct sockaddr_in& addr) : CNetAddr(addr.sin_addr), port(ntohs(addr.sin_port))
{
    assert(addr.sin_family == AF_INET);
}
CService::CService(const struct sockaddr_in6 &addr) : CNetAddr(addr.sin6_addr, addr.sin6_scope_id), port(ntohs(addr.sin6_port))
{
   assert(addr.sin6_family == AF_INET6);
}
bool CService::SetSockAddr(const struct sockaddr *paddr)
{
    switch (paddr->sa_family) {
    case AF_INET:
        *this = CService(*(const struct sockaddr_in*)paddr);
        return true;
    case AF_INET6:
        *this = CService(*(const struct sockaddr_in6*)paddr);
        return true;
    default:
        return false;
    }
}
uint16_t CService::GetPort() const
{
    return port;
}
bool operator==(const CService& a, const CService& b)
{
    return static_cast<CNetAddr>(a) == static_cast<CNetAddr>(b) && a.port == b.port;
}
bool operator<(const CService& a, const CService& b)
{
    return static_cast<CNetAddr>(a) < static_cast<CNetAddr>(b) || (static_cast<CNetAddr>(a) == static_cast<CNetAddr>(b) && a.port < b.port);
}
bool CService::GetSockAddr(struct sockaddr* paddr, socklen_t *addrlen) const
{
    if (IsIPv4()) {
        if (*addrlen < (socklen_t)sizeof(struct sockaddr_in))
            return false;
        *addrlen = sizeof(struct sockaddr_in);
        struct sockaddr_in *paddrin = (struct sockaddr_in*)paddr;
        memset(paddrin, 0, *addrlen);
        if (!GetInAddr(&paddrin->sin_addr))
            return false;
        paddrin->sin_family = AF_INET;
        paddrin->sin_port = htons(port);
        return true;
    }
    if (IsIPv6() || IsCJDNS()) {
        if (*addrlen < (socklen_t)sizeof(struct sockaddr_in6))
            return false;
        *addrlen = sizeof(struct sockaddr_in6);
        struct sockaddr_in6 *paddrin6 = (struct sockaddr_in6*)paddr;
        memset(paddrin6, 0, *addrlen);
        if (!GetIn6Addr(&paddrin6->sin6_addr))
            return false;
        paddrin6->sin6_scope_id = m_scope_id;
        paddrin6->sin6_family = AF_INET6;
        paddrin6->sin6_port = htons(port);
        return true;
    }
    return false;
}
std::vector<unsigned char> CService::GetKey() const
{
    auto key = GetAddrBytes();
    key.push_back(port / 0x100);
    key.push_back(port & 0x0FF);
    return key;
}
std::string CService::ToStringPort() const
{
    return strprintf("%u", port);
}
std::string CService::ToStringIPPort() const
{
    if (IsIPv4() || IsTor() || IsI2P() || IsInternal()) {
        return ToStringIP() + ":" + ToStringPort();
    } else {
        return "[" + ToStringIP() + "]:" + ToStringPort();
    }
}
std::string CService::ToString() const
{
    return ToStringIPPort();
}
CSubNet::CSubNet():
    valid(false)
{
    memset(netmask, 0, sizeof(netmask));
}
CSubNet::CSubNet(const CNetAddr& addr, uint8_t mask) : CSubNet()
{
    valid = (addr.IsIPv4() && mask <= ADDR_IPV4_SIZE * 8) ||
            (addr.IsIPv6() && mask <= ADDR_IPV6_SIZE * 8);
    if (!valid) {
        return;
    }
    assert(mask <= sizeof(netmask) * 8);
    network = addr;
    uint8_t n = mask;
    for (size_t i = 0; i < network.m_addr.size(); ++i) {
        const uint8_t bits = n < 8 ? n : 8;
        netmask[i] = (uint8_t)((uint8_t)0xFF << (8 - bits));
        network.m_addr[i] &= netmask[i];
        n -= bits;
    }
}
static inline int NetmaskBits(uint8_t x)
{
    switch(x) {
    case 0x00: return 0;
    case 0x80: return 1;
    case 0xc0: return 2;
    case 0xe0: return 3;
    case 0xf0: return 4;
    case 0xf8: return 5;
    case 0xfc: return 6;
    case 0xfe: return 7;
    case 0xff: return 8;
    default: return -1;
    }
}
CSubNet::CSubNet(const CNetAddr& addr, const CNetAddr& mask) : CSubNet()
{
    valid = (addr.IsIPv4() || addr.IsIPv6()) && addr.m_net == mask.m_net;
    if (!valid) {
        return;
    }
    bool zeros_found = false;
    for (auto b : mask.m_addr) {
        const int num_bits = NetmaskBits(b);
        if (num_bits == -1 || (zeros_found && num_bits != 0)) {
            valid = false;
            return;
        }
        if (num_bits < 8) {
            zeros_found = true;
        }
    }
    assert(mask.m_addr.size() <= sizeof(netmask));
    memcpy(netmask, mask.m_addr.data(), mask.m_addr.size());
    network = addr;
    for (size_t x = 0; x < network.m_addr.size(); ++x) {
        network.m_addr[x] &= netmask[x];
    }
}
CSubNet::CSubNet(const CNetAddr& addr) : CSubNet()
{
    switch (addr.m_net) {
    case NET_IPV4:
    case NET_IPV6:
        valid = true;
        assert(addr.m_addr.size() <= sizeof(netmask));
        memset(netmask, 0xFF, addr.m_addr.size());
        break;
    case NET_ONION:
    case NET_I2P:
    case NET_CJDNS:
        valid = true;
        break;
    case NET_INTERNAL:
    case NET_UNROUTABLE:
    case NET_MAX:
        return;
    }
    network = addr;
}
bool CSubNet::Match(const CNetAddr &addr) const
{
    if (!valid || !addr.IsValid() || network.m_net != addr.m_net)
        return false;
    switch (network.m_net) {
    case NET_IPV4:
    case NET_IPV6:
        break;
    case NET_ONION:
    case NET_I2P:
    case NET_CJDNS:
    case NET_INTERNAL:
        return addr == network;
    case NET_UNROUTABLE:
    case NET_MAX:
        return false;
    }
    assert(network.m_addr.size() == addr.m_addr.size());
    for (size_t x = 0; x < addr.m_addr.size(); ++x) {
        if ((addr.m_addr[x] & netmask[x]) != network.m_addr[x]) {
            return false;
        }
    }
    return true;
}
std::string CSubNet::ToString() const
{
    std::string suffix;
    switch (network.m_net) {
    case NET_IPV4:
    case NET_IPV6: {
        assert(network.m_addr.size() <= sizeof(netmask));
        uint8_t cidr = 0;
        for (size_t i = 0; i < network.m_addr.size(); ++i) {
            if (netmask[i] == 0x00) {
                break;
            }
            cidr += NetmaskBits(netmask[i]);
        }
        suffix = strprintf("/%u", cidr);
        break;
    }
    case NET_ONION:
    case NET_I2P:
    case NET_CJDNS:
    case NET_INTERNAL:
    case NET_UNROUTABLE:
    case NET_MAX:
        break;
    }
    return network.ToString() + suffix;
}
bool CSubNet::IsValid() const
{
    return valid;
}
bool CSubNet::SanityCheck() const
{
    switch (network.m_net) {
    case NET_IPV4:
    case NET_IPV6:
        break;
    case NET_ONION:
    case NET_I2P:
    case NET_CJDNS:
        return true;
    case NET_INTERNAL:
    case NET_UNROUTABLE:
    case NET_MAX:
        return false;
    }
    for (size_t x = 0; x < network.m_addr.size(); ++x) {
        if (network.m_addr[x] & ~netmask[x]) return false;
    }
    return true;
}
bool operator==(const CSubNet& a, const CSubNet& b)
{
    return a.valid == b.valid && a.network == b.network && !memcmp(a.netmask, b.netmask, 16);
}
bool operator<(const CSubNet& a, const CSubNet& b)
{
    return (a.network < b.network || (a.network == b.network && memcmp(a.netmask, b.netmask, 16) < 0));
}
