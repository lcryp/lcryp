#ifndef LCRYP_UTIL_BYTEVECTORHASH_H
#define LCRYP_UTIL_BYTEVECTORHASH_H
#include <cstdint>
#include <cstddef>
#include <vector>
class ByteVectorHash final
{
private:
    uint64_t m_k0, m_k1;
public:
    ByteVectorHash();
    size_t operator()(const std::vector<unsigned char>& input) const;
};
#endif
