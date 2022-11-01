#ifndef LCRYP_RANDOM_H
#define LCRYP_RANDOM_H
#include <crypto/chacha20.h>
#include <crypto/common.h>
#include <span.h>
#include <uint256.h>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <limits>
void GetRandBytes(Span<unsigned char> bytes) noexcept;
uint64_t GetRandInternal(uint64_t nMax) noexcept;
template<typename T>
T GetRand(T nMax=std::numeric_limits<T>::max()) noexcept {
    static_assert(std::is_integral<T>(), "T must be integral");
    static_assert(std::numeric_limits<T>::max() <= std::numeric_limits<uint64_t>::max(), "GetRand only supports up to uint64_t");
    return T(GetRandInternal(nMax));
}
template <typename D>
D GetRandomDuration(typename std::common_type<D>::type max) noexcept
{
    assert(max.count() > 0);
    return D{GetRand(max.count())};
};
constexpr auto GetRandMicros = GetRandomDuration<std::chrono::microseconds>;
constexpr auto GetRandMillis = GetRandomDuration<std::chrono::milliseconds>;
std::chrono::microseconds GetExponentialRand(std::chrono::microseconds now, std::chrono::seconds average_interval);
uint256 GetRandHash() noexcept;
void GetStrongRandBytes(Span<unsigned char> bytes) noexcept;
void RandAddPeriodic() noexcept;
void RandAddEvent(const uint32_t event_info) noexcept;
class FastRandomContext
{
private:
    bool requires_seed;
    ChaCha20 rng;
    unsigned char bytebuf[64];
    int bytebuf_size;
    uint64_t bitbuf;
    int bitbuf_size;
    void RandomSeed();
    void FillByteBuffer()
    {
        if (requires_seed) {
            RandomSeed();
        }
        rng.Keystream(bytebuf, sizeof(bytebuf));
        bytebuf_size = sizeof(bytebuf);
    }
    void FillBitBuffer()
    {
        bitbuf = rand64();
        bitbuf_size = 64;
    }
public:
    explicit FastRandomContext(bool fDeterministic = false) noexcept;
    explicit FastRandomContext(const uint256& seed) noexcept;
    FastRandomContext(const FastRandomContext&) = delete;
    FastRandomContext(FastRandomContext&&) = delete;
    FastRandomContext& operator=(const FastRandomContext&) = delete;
    FastRandomContext& operator=(FastRandomContext&& from) noexcept;
    uint64_t rand64() noexcept
    {
        if (bytebuf_size < 8) FillByteBuffer();
        uint64_t ret = ReadLE64(bytebuf + 64 - bytebuf_size);
        bytebuf_size -= 8;
        return ret;
    }
    uint64_t randbits(int bits) noexcept
    {
        if (bits == 0) {
            return 0;
        } else if (bits > 32) {
            return rand64() >> (64 - bits);
        } else {
            if (bitbuf_size < bits) FillBitBuffer();
            uint64_t ret = bitbuf & (~(uint64_t)0 >> (64 - bits));
            bitbuf >>= bits;
            bitbuf_size -= bits;
            return ret;
        }
    }
    uint64_t randrange(uint64_t range) noexcept
    {
        assert(range);
        --range;
        int bits = CountBits(range);
        while (true) {
            uint64_t ret = randbits(bits);
            if (ret <= range) return ret;
        }
    }
    std::vector<unsigned char> randbytes(size_t len);
    uint32_t rand32() noexcept { return randbits(32); }
    uint256 rand256() noexcept;
    bool randbool() noexcept { return randbits(1); }
    template <typename Tp>
    Tp rand_uniform_delay(const Tp& time, typename Tp::duration range)
    {
        return time + rand_uniform_duration<Tp>(range);
    }
    template <typename Chrono>
    typename Chrono::duration rand_uniform_duration(typename Chrono::duration range) noexcept
    {
        using Dur = typename Chrono::duration;
        return range.count() > 0 ?   Dur{randrange(range.count())} :
               range.count() < 0 ?   -Dur{randrange(-range.count())} :
                                     Dur{0};
    };
    typedef uint64_t result_type;
    static constexpr uint64_t min() { return 0; }
    static constexpr uint64_t max() { return std::numeric_limits<uint64_t>::max(); }
    inline uint64_t operator()() noexcept { return rand64(); }
};
template <typename I, typename R>
void Shuffle(I first, I last, R&& rng)
{
    while (first != last) {
        size_t j = rng.randrange(last - first);
        if (j) {
            using std::swap;
            swap(*first, *(first + j));
        }
        ++first;
    }
}
static const int NUM_OS_RANDOM_BYTES = 32;
void GetOSRand(unsigned char* ent32);
bool Random_SanityCheck();
void RandomInit();
#endif
