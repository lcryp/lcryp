#include <netgroup.h>
#include <hash.h>
#include <util/asmap.h>
uint256 NetGroupManager::GetAsmapChecksum() const
{
    if (!m_asmap.size()) return {};
    return SerializeHash(m_asmap);
}
std::vector<unsigned char> NetGroupManager::GetGroup(const CNetAddr& address) const
{
    std::vector<unsigned char> vchRet;
    uint32_t asn = GetMappedAS(address);
    if (asn != 0) {
        vchRet.push_back(NET_IPV6);
        for (int i = 0; i < 4; i++) {
            vchRet.push_back((asn >> (8 * i)) & 0xFF);
        }
        return vchRet;
    }
    vchRet.push_back(address.GetNetClass());
    int nStartByte{0};
    int nBits{0};
    if (address.IsLocal()) {
    } else if (address.IsInternal()) {
        nStartByte = INTERNAL_IN_IPV6_PREFIX.size();
        nBits = ADDR_INTERNAL_SIZE * 8;
    } else if (!address.IsRoutable()) {
    } else if (address.HasLinkedIPv4()) {
        uint32_t ipv4 = address.GetLinkedIPv4();
        vchRet.push_back((ipv4 >> 24) & 0xFF);
        vchRet.push_back((ipv4 >> 16) & 0xFF);
        return vchRet;
    } else if (address.IsTor() || address.IsI2P()) {
        nBits = 4;
    } else if (address.IsCJDNS()) {
        nBits = 12;
    } else if (address.IsHeNet()) {
        nBits = 36;
    } else {
        nBits = 32;
    }
    auto addr_bytes = address.GetAddrBytes();
    const size_t num_bytes = nBits / 8;
    vchRet.insert(vchRet.end(), addr_bytes.begin() + nStartByte, addr_bytes.begin() + nStartByte + num_bytes);
    nBits %= 8;
    if (nBits > 0) {
        assert(num_bytes < addr_bytes.size());
        vchRet.push_back(addr_bytes[num_bytes + nStartByte] | ((1 << (8 - nBits)) - 1));
    }
    return vchRet;
}
uint32_t NetGroupManager::GetMappedAS(const CNetAddr& address) const
{
    uint32_t net_class = address.GetNetClass();
    if (m_asmap.size() == 0 || (net_class != NET_IPV4 && net_class != NET_IPV6)) {
        return 0;
    }
    std::vector<bool> ip_bits(128);
    if (address.HasLinkedIPv4()) {
        for (int8_t byte_i = 0; byte_i < 12; ++byte_i) {
            for (uint8_t bit_i = 0; bit_i < 8; ++bit_i) {
                ip_bits[byte_i * 8 + bit_i] = (IPV4_IN_IPV6_PREFIX[byte_i] >> (7 - bit_i)) & 1;
            }
        }
        uint32_t ipv4 = address.GetLinkedIPv4();
        for (int i = 0; i < 32; ++i) {
            ip_bits[96 + i] = (ipv4 >> (31 - i)) & 1;
        }
    } else {
        assert(address.IsIPv6());
        auto addr_bytes = address.GetAddrBytes();
        for (int8_t byte_i = 0; byte_i < 16; ++byte_i) {
            uint8_t cur_byte = addr_bytes[byte_i];
            for (uint8_t bit_i = 0; bit_i < 8; ++bit_i) {
                ip_bits[byte_i * 8 + bit_i] = (cur_byte >> (7 - bit_i)) & 1;
            }
        }
    }
    uint32_t mapped_as = Interpret(m_asmap, ip_bits);
    return mapped_as;
}
