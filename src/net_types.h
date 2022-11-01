#ifndef LCRYP_NET_TYPES_H
#define LCRYP_NET_TYPES_H
#include <cstdint>
#include <map>
class CSubNet;
class UniValue;
class CBanEntry
{
public:
    static constexpr int CURRENT_VERSION{1};
    int nVersion{CBanEntry::CURRENT_VERSION};
    int64_t nCreateTime{0};
    int64_t nBanUntil{0};
    CBanEntry() {}
    explicit CBanEntry(int64_t nCreateTimeIn)
        : nCreateTime{nCreateTimeIn} {}
    explicit CBanEntry(const UniValue& json);
    UniValue ToJson() const;
};
using banmap_t = std::map<CSubNet, CBanEntry>;
UniValue BanMapToJson(const banmap_t& bans);
void BanMapFromJson(const UniValue& bans_json, banmap_t& bans);
#endif
