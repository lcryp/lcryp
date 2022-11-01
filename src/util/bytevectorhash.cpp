#include <crypto/siphash.h>
#include <random.h>
#include <util/bytevectorhash.h>
#include <vector>
ByteVectorHash::ByteVectorHash() :
    m_k0(GetRand<uint64_t>()),
    m_k1(GetRand<uint64_t>())
{
}
size_t ByteVectorHash::operator()(const std::vector<unsigned char>& input) const
{
    return CSipHasher(m_k0, m_k1).Write(input.data(), input.size()).Finalize();
}
