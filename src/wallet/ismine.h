#ifndef LCRYP_WALLET_ISMINE_H
#define LCRYP_WALLET_ISMINE_H
#include <script/standard.h>
#include <bitset>
#include <cstdint>
#include <type_traits>
class CScript;
namespace wallet {
class CWallet;
enum isminetype : unsigned int {
    ISMINE_NO         = 0,
    ISMINE_WATCH_ONLY = 1 << 0,
    ISMINE_SPENDABLE  = 1 << 1,
    ISMINE_USED       = 1 << 2,
    ISMINE_ALL        = ISMINE_WATCH_ONLY | ISMINE_SPENDABLE,
    ISMINE_ALL_USED   = ISMINE_ALL | ISMINE_USED,
    ISMINE_ENUM_ELEMENTS,
};
using isminefilter = std::underlying_type<isminetype>::type;
struct CachableAmount
{
    std::bitset<ISMINE_ENUM_ELEMENTS> m_cached;
    CAmount m_value[ISMINE_ENUM_ELEMENTS];
    inline void Reset()
    {
        m_cached.reset();
    }
    void Set(isminefilter filter, CAmount value)
    {
        m_cached.set(filter);
        m_value[filter] = value;
    }
};
}
#endif
