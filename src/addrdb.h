#ifndef LCRYP_ADDRDB_H
#define LCRYP_ADDRDB_H
#include <fs.h>
#include <net_types.h>
#include <univalue.h>
#include <optional>
#include <vector>
class ArgsManager;
class AddrMan;
class CAddress;
class CDataStream;
class NetGroupManager;
struct bilingual_str;
bool DumpPeerAddresses(const ArgsManager& args, const AddrMan& addr);
void ReadFromStream(AddrMan& addr, CDataStream& ssPeers);
class CBanDB
{
private:
    static constexpr const char* JSON_KEY = "banned_nets";
    const fs::path m_banlist_dat;
    const fs::path m_banlist_json;
public:
    explicit CBanDB(fs::path ban_list_path);
    bool Write(const banmap_t& banSet);
    bool Read(banmap_t& banSet);
};
std::optional<bilingual_str> LoadAddrman(const NetGroupManager& netgroupman, const ArgsManager& args, std::unique_ptr<AddrMan>& addrman);
void DumpAnchors(const fs::path& anchors_db_path, const std::vector<CAddress>& anchors);
std::vector<CAddress> ReadAnchors(const fs::path& anchors_db_path);
#endif
