#ifndef LCRYP_NETGROUP_H
#define LCRYP_NETGROUP_H
#include <netaddress.h>
#include <uint256.h>
#include <vector>
class NetGroupManager {
public:
    explicit NetGroupManager(std::vector<bool> asmap)
        : m_asmap{std::move(asmap)}
    {}
    uint256 GetAsmapChecksum() const;
    std::vector<unsigned char> GetGroup(const CNetAddr& address) const;
    uint32_t GetMappedAS(const CNetAddr& address) const;
private:
    const std::vector<bool> m_asmap;
};
#endif
