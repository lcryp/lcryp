#ifndef LCRYP_ADDRMAN_H
#define LCRYP_ADDRMAN_H
#include <netaddress.h>
#include <netgroup.h>
#include <protocol.h>
#include <streams.h>
#include <util/time.h>
#include <cstdint>
#include <memory>
#include <optional>
#include <utility>
#include <vector>
class InvalidAddrManVersionError : public std::ios_base::failure
{
public:
    InvalidAddrManVersionError(std::string msg) : std::ios_base::failure(msg) { }
};
class AddrManImpl;
static constexpr int32_t DEFAULT_ADDRMAN_CONSISTENCY_CHECKS{0};
struct AddressPosition {
    const bool tried;
    const int multiplicity;
    const int bucket;
    const int position;
    bool operator==(AddressPosition other) {
        return std::tie(tried, multiplicity, bucket, position) ==
               std::tie(other.tried, other.multiplicity, other.bucket, other.position);
    }
    explicit AddressPosition(bool tried_in, int multiplicity_in, int bucket_in, int position_in)
        : tried{tried_in}, multiplicity{multiplicity_in}, bucket{bucket_in}, position{position_in} {}
};
class AddrMan
{
protected:
    const std::unique_ptr<AddrManImpl> m_impl;
public:
    explicit AddrMan(const NetGroupManager& netgroupman, bool deterministic, int32_t consistency_check_ratio);
    ~AddrMan();
    template <typename Stream>
    void Serialize(Stream& s_) const;
    template <typename Stream>
    void Unserialize(Stream& s_);
    size_t size() const;
    bool Add(const std::vector<CAddress>& vAddr, const CNetAddr& source, std::chrono::seconds time_penalty = 0s);
    bool Good(const CService& addr, NodeSeconds time = Now<NodeSeconds>());
    void Attempt(const CService& addr, bool fCountFailure, NodeSeconds time = Now<NodeSeconds>());
    void ResolveCollisions();
    std::pair<CAddress, NodeSeconds> SelectTriedCollision();
    std::pair<CAddress, NodeSeconds> Select(bool newOnly = false) const;
    std::vector<CAddress> GetAddr(size_t max_addresses, size_t max_pct, std::optional<Network> network) const;
    void Connected(const CService& addr, NodeSeconds time = Now<NodeSeconds>());
    void SetServices(const CService& addr, ServiceFlags nServices);
    std::optional<AddressPosition> FindAddressEntry(const CAddress& addr);
};
#endif
