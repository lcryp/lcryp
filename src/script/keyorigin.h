#ifndef LCRYP_SCRIPT_KEYORIGIN_H
#define LCRYP_SCRIPT_KEYORIGIN_H
#include <serialize.h>
#include <vector>
struct KeyOriginInfo
{
    unsigned char fingerprint[4];
    std::vector<uint32_t> path;
    friend bool operator==(const KeyOriginInfo& a, const KeyOriginInfo& b)
    {
        return std::equal(std::begin(a.fingerprint), std::end(a.fingerprint), std::begin(b.fingerprint)) && a.path == b.path;
    }
    friend bool operator<(const KeyOriginInfo& a, const KeyOriginInfo& b)
    {
        int fpr_cmp = memcmp(a.fingerprint, b.fingerprint, 4);
        if (fpr_cmp < 0) {
            return true;
        } else if (fpr_cmp > 0) {
            return false;
        }
        if (a.path.size() < b.path.size()) {
            return true;
        } else if (a.path.size() > b.path.size()) {
            return false;
        }
        return a.path < b.path;
    }
    SERIALIZE_METHODS(KeyOriginInfo, obj) { READWRITE(obj.fingerprint, obj.path); }
    void clear()
    {
        memset(fingerprint, 0, 4);
        path.clear();
    }
};
#endif
