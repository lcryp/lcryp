#ifndef LCRYP_BANMAN_H
#define LCRYP_BANMAN_H
#include <addrdb.h>
#include <common/bloom.h>
#include <fs.h>
#include <net_types.h>
#include <sync.h>
#include <chrono>
#include <cstdint>
#include <memory>
static constexpr unsigned int DEFAULT_MISBEHAVING_BANTIME = 60 * 60 * 24;
static constexpr std::chrono::minutes DUMP_BANS_INTERVAL{15};
class CClientUIInterface;
class CNetAddr;
class CSubNet;
class BanMan
{
public:
    ~BanMan();
    BanMan(fs::path ban_file, CClientUIInterface* client_interface, int64_t default_ban_time);
    void Ban(const CNetAddr& net_addr, int64_t ban_time_offset = 0, bool since_unix_epoch = false);
    void Ban(const CSubNet& sub_net, int64_t ban_time_offset = 0, bool since_unix_epoch = false);
    void Discourage(const CNetAddr& net_addr);
    void ClearBanned();
    bool IsBanned(const CNetAddr& net_addr);
    bool IsBanned(const CSubNet& sub_net);
    bool IsDiscouraged(const CNetAddr& net_addr);
    bool Unban(const CNetAddr& net_addr);
    bool Unban(const CSubNet& sub_net);
    void GetBanned(banmap_t& banmap);
    void DumpBanlist();
private:
    void LoadBanlist() EXCLUSIVE_LOCKS_REQUIRED(!m_cs_banned);
    bool BannedSetIsDirty();
    void SetBannedSetDirty(bool dirty = true);
    void SweepBanned() EXCLUSIVE_LOCKS_REQUIRED(m_cs_banned);
    RecursiveMutex m_cs_banned;
    banmap_t m_banned GUARDED_BY(m_cs_banned);
    bool m_is_dirty GUARDED_BY(m_cs_banned){false};
    CClientUIInterface* m_client_interface = nullptr;
    CBanDB m_ban_db;
    const int64_t m_default_ban_time;
    CRollingBloomFilter m_discouraged GUARDED_BY(m_cs_banned) {50000, 0.000001};
};
#endif
