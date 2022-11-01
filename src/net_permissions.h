#include <netaddress.h>
#include <string>
#include <type_traits>
#include <vector>
#ifndef LCRYP_NET_PERMISSIONS_H
#define LCRYP_NET_PERMISSIONS_H
struct bilingual_str;
extern const std::vector<std::string> NET_PERMISSIONS_DOC;
enum class NetPermissionFlags : uint32_t {
    None = 0,
    BloomFilter = (1U << 1),
    Relay = (1U << 3),
    ForceRelay = (1U << 2) | Relay,
    Download = (1U << 6),
    NoBan = (1U << 4) | Download,
    Mempool = (1U << 5),
    Addr = (1U << 7),
    Implicit = (1U << 31),
    All = BloomFilter | ForceRelay | Relay | NoBan | Mempool | Download | Addr,
};
static inline constexpr NetPermissionFlags operator|(NetPermissionFlags a, NetPermissionFlags b)
{
    using t = typename std::underlying_type<NetPermissionFlags>::type;
    return static_cast<NetPermissionFlags>(static_cast<t>(a) | static_cast<t>(b));
}
class NetPermissions
{
public:
    NetPermissionFlags m_flags;
    static std::vector<std::string> ToStrings(NetPermissionFlags flags);
    static inline bool HasFlag(NetPermissionFlags flags, NetPermissionFlags f)
    {
        using t = typename std::underlying_type<NetPermissionFlags>::type;
        return (static_cast<t>(flags) & static_cast<t>(f)) == static_cast<t>(f);
    }
    static inline void AddFlag(NetPermissionFlags& flags, NetPermissionFlags f)
    {
        flags = flags | f;
    }
    static inline void ClearFlag(NetPermissionFlags& flags, NetPermissionFlags f)
    {
        assert(f == NetPermissionFlags::Implicit);
        using t = typename std::underlying_type<NetPermissionFlags>::type;
        flags = static_cast<NetPermissionFlags>(static_cast<t>(flags) & ~static_cast<t>(f));
    }
};
class NetWhitebindPermissions : public NetPermissions
{
public:
    static bool TryParse(const std::string& str, NetWhitebindPermissions& output, bilingual_str& error);
    CService m_service;
};
class NetWhitelistPermissions : public NetPermissions
{
public:
    static bool TryParse(const std::string& str, NetWhitelistPermissions& output, bilingual_str& error);
    CSubNet m_subnet;
};
#endif
