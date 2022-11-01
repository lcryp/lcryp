#include <tinyformat.h>
#include <util/bip32.h>
#include <util/strencodings.h>
#include <cstdint>
#include <cstdio>
#include <sstream>
bool ParseHDKeypath(const std::string& keypath_str, std::vector<uint32_t>& keypath)
{
    std::stringstream ss(keypath_str);
    std::string item;
    bool first = true;
    while (std::getline(ss, item, '/')) {
        if (item.compare("m") == 0) {
            if (first) {
                first = false;
                continue;
            }
            return false;
        }
        uint32_t path = 0;
        size_t pos = item.find("'");
        if (pos != std::string::npos) {
            if (pos != item.size() - 1) {
                return false;
            }
            path |= 0x80000000;
            item = item.substr(0, item.size() - 1);
        }
        if (item.find_first_not_of( "0123456789" ) != std::string::npos) {
            return false;
        }
        uint32_t number;
        if (!ParseUInt32(item, &number)) {
            return false;
        }
        path |= number;
        keypath.push_back(path);
        first = false;
    }
    return true;
}
std::string FormatHDKeypath(const std::vector<uint32_t>& path)
{
    std::string ret;
    for (auto i : path) {
        ret += strprintf("/%i", (i << 1) >> 1);
        if (i >> 31) ret += '\'';
    }
    return ret;
}
std::string WriteHDKeypath(const std::vector<uint32_t>& keypath)
{
    return "m" + FormatHDKeypath(keypath);
}
