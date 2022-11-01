#ifndef LCRYP_UTIL_GOLOMBRICE_H
#define LCRYP_UTIL_GOLOMBRICE_H
#include <util/fastrange.h>
#include <streams.h>
#include <cstdint>
template <typename OStream>
void GolombRiceEncode(BitStreamWriter<OStream>& bitwriter, uint8_t P, uint64_t x)
{
    uint64_t q = x >> P;
    while (q > 0) {
        int nbits = q <= 64 ? static_cast<int>(q) : 64;
        bitwriter.Write(~0ULL, nbits);
        q -= nbits;
    }
    bitwriter.Write(0, 1);
    bitwriter.Write(x, P);
}
template <typename IStream>
uint64_t GolombRiceDecode(BitStreamReader<IStream>& bitreader, uint8_t P)
{
    uint64_t q = 0;
    while (bitreader.Read(1) == 1) {
        ++q;
    }
    uint64_t r = bitreader.Read(P);
    return (q << P) + r;
}
#endif
