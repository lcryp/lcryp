#include <random.h>
#include <compat/cpuid.h>
#include <crypto/sha256.h>
#include <crypto/sha512.h>
#include <support/cleanse.h>
#ifdef WIN32
#include <compat/compat.h>
#include <wincrypt.h>
#endif
#include <logging.h>
#include <randomenv.h>
#include <support/allocators/secure.h>
#include <span.h>
#include <sync.h>
#include <util/time.h>
#include <cmath>
#include <cstdlib>
#include <thread>
#ifndef WIN32
#include <fcntl.h>
#include <sys/time.h>
#endif
#ifdef HAVE_SYS_GETRANDOM
#include <sys/syscall.h>
#include <linux/random.h>
#endif
#if defined(HAVE_GETENTROPY_RAND) && defined(MAC_OSX)
#include <unistd.h>
#include <sys/random.h>
#endif
#ifdef HAVE_SYSCTL_ARND
#include <sys/sysctl.h>
#endif
[[noreturn]] static void RandFailure()
{
    LogPrintf("Failed to read randomness, aborting\n");
    std::abort();
}
static inline int64_t GetPerformanceCounter() noexcept
{
#if defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))
    return __rdtsc();
#elif !defined(_MSC_VER) && defined(__i386__)
    uint64_t r = 0;
    __asm__ volatile ("rdtsc" : "=A"(r));
    return r;
#elif !defined(_MSC_VER) && (defined(__x86_64__) || defined(__amd64__))
    uint64_t r1 = 0, r2 = 0;
    __asm__ volatile ("rdtsc" : "=a"(r1), "=d"(r2));
    return (r2 << 32) | r1;
#else
    return std::chrono::high_resolution_clock::now().time_since_epoch().count();
#endif
}
#ifdef HAVE_GETCPUID
static bool g_rdrand_supported = false;
static bool g_rdseed_supported = false;
static constexpr uint32_t CPUID_F1_ECX_RDRAND = 0x40000000;
static constexpr uint32_t CPUID_F7_EBX_RDSEED = 0x00040000;
#ifdef bit_RDRND
static_assert(CPUID_F1_ECX_RDRAND == bit_RDRND, "Unexpected value for bit_RDRND");
#endif
#ifdef bit_RDSEED
static_assert(CPUID_F7_EBX_RDSEED == bit_RDSEED, "Unexpected value for bit_RDSEED");
#endif
static void InitHardwareRand()
{
    uint32_t eax, ebx, ecx, edx;
    GetCPUID(1, 0, eax, ebx, ecx, edx);
    if (ecx & CPUID_F1_ECX_RDRAND) {
        g_rdrand_supported = true;
    }
    GetCPUID(7, 0, eax, ebx, ecx, edx);
    if (ebx & CPUID_F7_EBX_RDSEED) {
        g_rdseed_supported = true;
    }
}
static void ReportHardwareRand()
{
    if (g_rdseed_supported) {
        LogPrintf("Using RdSeed as an additional entropy source\n");
    }
    if (g_rdrand_supported) {
        LogPrintf("Using RdRand as an additional entropy source\n");
    }
}
static uint64_t GetRdRand() noexcept
{
#ifdef __i386__
    uint8_t ok;
    uint32_t r1 = 0, r2 = 0;
    for (int i = 0; i < 10; ++i) {
        __asm__ volatile (".byte 0x0f, 0xc7, 0xf0; setc %1" : "=a"(r1), "=q"(ok) :: "cc");
        if (ok) break;
    }
    for (int i = 0; i < 10; ++i) {
        __asm__ volatile (".byte 0x0f, 0xc7, 0xf0; setc %1" : "=a"(r2), "=q"(ok) :: "cc");
        if (ok) break;
    }
    return (((uint64_t)r2) << 32) | r1;
#elif defined(__x86_64__) || defined(__amd64__)
    uint8_t ok;
    uint64_t r1 = 0;
    for (int i = 0; i < 10; ++i) {
        __asm__ volatile (".byte 0x48, 0x0f, 0xc7, 0xf0; setc %1" : "=a"(r1), "=q"(ok) :: "cc");
        if (ok) break;
    }
    return r1;
#else
#error "RdRand is only supported on x86 and x86_64"
#endif
}
static uint64_t GetRdSeed() noexcept
{
#ifdef __i386__
    uint8_t ok;
    uint32_t r1, r2;
    do {
        __asm__ volatile (".byte 0x0f, 0xc7, 0xf8; setc %1" : "=a"(r1), "=q"(ok) :: "cc");
        if (ok) break;
        __asm__ volatile ("pause");
    } while(true);
    do {
        __asm__ volatile (".byte 0x0f, 0xc7, 0xf8; setc %1" : "=a"(r2), "=q"(ok) :: "cc");
        if (ok) break;
        __asm__ volatile ("pause");
    } while(true);
    return (((uint64_t)r2) << 32) | r1;
#elif defined(__x86_64__) || defined(__amd64__)
    uint8_t ok;
    uint64_t r1;
    do {
        __asm__ volatile (".byte 0x48, 0x0f, 0xc7, 0xf8; setc %1" : "=a"(r1), "=q"(ok) :: "cc");
        if (ok) break;
        __asm__ volatile ("pause");
    } while(true);
    return r1;
#else
#error "RdSeed is only supported on x86 and x86_64"
#endif
}
#else
static void InitHardwareRand() {}
static void ReportHardwareRand() {}
#endif
static void SeedHardwareFast(CSHA512& hasher) noexcept {
#if defined(__x86_64__) || defined(__amd64__) || defined(__i386__)
    if (g_rdrand_supported) {
        uint64_t out = GetRdRand();
        hasher.Write((const unsigned char*)&out, sizeof(out));
        return;
    }
#endif
}
static void SeedHardwareSlow(CSHA512& hasher) noexcept {
#if defined(__x86_64__) || defined(__amd64__) || defined(__i386__)
    if (g_rdseed_supported) {
        for (int i = 0; i < 4; ++i) {
            uint64_t out = GetRdSeed();
            hasher.Write((const unsigned char*)&out, sizeof(out));
        }
        return;
    }
    if (g_rdrand_supported) {
        for (int i = 0; i < 4; ++i) {
            uint64_t out = 0;
            for (int j = 0; j < 1024; ++j) out ^= GetRdRand();
            hasher.Write((const unsigned char*)&out, sizeof(out));
        }
        return;
    }
#endif
}
static void Strengthen(const unsigned char (&seed)[32], int microseconds, CSHA512& hasher) noexcept
{
    CSHA512 inner_hasher;
    inner_hasher.Write(seed, sizeof(seed));
    unsigned char buffer[64];
    int64_t stop = GetTimeMicros() + microseconds;
    do {
        for (int i = 0; i < 1000; ++i) {
            inner_hasher.Finalize(buffer);
            inner_hasher.Reset();
            inner_hasher.Write(buffer, sizeof(buffer));
        }
        int64_t perf = GetPerformanceCounter();
        hasher.Write((const unsigned char*)&perf, sizeof(perf));
    } while (GetTimeMicros() < stop);
    inner_hasher.Finalize(buffer);
    hasher.Write(buffer, sizeof(buffer));
    inner_hasher.Reset();
    memory_cleanse(buffer, sizeof(buffer));
}
#ifndef WIN32
static void GetDevURandom(unsigned char *ent32)
{
    int f = open("/dev/urandom", O_RDONLY);
    if (f == -1) {
        RandFailure();
    }
    int have = 0;
    do {
        ssize_t n = read(f, ent32 + have, NUM_OS_RANDOM_BYTES - have);
        if (n <= 0 || n + have > NUM_OS_RANDOM_BYTES) {
            close(f);
            RandFailure();
        }
        have += n;
    } while (have < NUM_OS_RANDOM_BYTES);
    close(f);
}
#endif
void GetOSRand(unsigned char *ent32)
{
#if defined(WIN32)
    HCRYPTPROV hProvider;
    int ret = CryptAcquireContextW(&hProvider, nullptr, nullptr, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT);
    if (!ret) {
        RandFailure();
    }
    ret = CryptGenRandom(hProvider, NUM_OS_RANDOM_BYTES, ent32);
    if (!ret) {
        RandFailure();
    }
    CryptReleaseContext(hProvider, 0);
#elif defined(HAVE_SYS_GETRANDOM)
    int rv = syscall(SYS_getrandom, ent32, NUM_OS_RANDOM_BYTES, 0);
    if (rv != NUM_OS_RANDOM_BYTES) {
        if (rv < 0 && errno == ENOSYS) {
            GetDevURandom(ent32);
        } else {
            RandFailure();
        }
    }
#elif defined(__OpenBSD__)
    arc4random_buf(ent32, NUM_OS_RANDOM_BYTES);
    (void)GetDevURandom;
#elif defined(HAVE_GETENTROPY_RAND) && defined(MAC_OSX)
    if (getentropy(ent32, NUM_OS_RANDOM_BYTES) != 0) {
        RandFailure();
    }
    (void)GetDevURandom;
#elif defined(HAVE_SYSCTL_ARND)
    static int name[2] = {CTL_KERN, KERN_ARND};
    int have = 0;
    do {
        size_t len = NUM_OS_RANDOM_BYTES - have;
        if (sysctl(name, std::size(name), ent32 + have, &len, nullptr, 0) != 0) {
            RandFailure();
        }
        have += len;
    } while (have < NUM_OS_RANDOM_BYTES);
    (void)GetDevURandom;
#else
    GetDevURandom(ent32);
#endif
}
namespace {
class RNGState {
    Mutex m_mutex;
    unsigned char m_state[32] GUARDED_BY(m_mutex) = {0};
    uint64_t m_counter GUARDED_BY(m_mutex) = 0;
    bool m_strongly_seeded GUARDED_BY(m_mutex) = false;
    Mutex m_events_mutex;
    CSHA256 m_events_hasher GUARDED_BY(m_events_mutex);
public:
    RNGState() noexcept
    {
        InitHardwareRand();
    }
    ~RNGState() = default;
    void AddEvent(uint32_t event_info) noexcept EXCLUSIVE_LOCKS_REQUIRED(!m_events_mutex)
    {
        LOCK(m_events_mutex);
        m_events_hasher.Write((const unsigned char *)&event_info, sizeof(event_info));
        uint32_t perfcounter = (GetPerformanceCounter() & 0xffffffff);
        m_events_hasher.Write((const unsigned char*)&perfcounter, sizeof(perfcounter));
    }
    void SeedEvents(CSHA512& hasher) noexcept EXCLUSIVE_LOCKS_REQUIRED(!m_events_mutex)
    {
        LOCK(m_events_mutex);
        unsigned char events_hash[32];
        m_events_hasher.Finalize(events_hash);
        hasher.Write(events_hash, 32);
        m_events_hasher.Reset();
        m_events_hasher.Write(events_hash, 32);
    }
    bool MixExtract(unsigned char* out, size_t num, CSHA512&& hasher, bool strong_seed) noexcept EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
    {
        assert(num <= 32);
        unsigned char buf[64];
        static_assert(sizeof(buf) == CSHA512::OUTPUT_SIZE, "Buffer needs to have hasher's output size");
        bool ret;
        {
            LOCK(m_mutex);
            ret = (m_strongly_seeded |= strong_seed);
            hasher.Write(m_state, 32);
            hasher.Write((const unsigned char*)&m_counter, sizeof(m_counter));
            ++m_counter;
            hasher.Finalize(buf);
            memcpy(m_state, buf + 32, 32);
        }
        if (num) {
            assert(out != nullptr);
            memcpy(out, buf, num);
        }
        hasher.Reset();
        memory_cleanse(buf, 64);
        return ret;
    }
};
RNGState& GetRNGState() noexcept
{
    static std::vector<RNGState, secure_allocator<RNGState>> g_rng(1);
    return g_rng[0];
}
}
static void SeedTimestamp(CSHA512& hasher) noexcept
{
    int64_t perfcounter = GetPerformanceCounter();
    hasher.Write((const unsigned char*)&perfcounter, sizeof(perfcounter));
}
static void SeedFast(CSHA512& hasher) noexcept
{
    unsigned char buffer[32];
    const unsigned char* ptr = buffer;
    hasher.Write((const unsigned char*)&ptr, sizeof(ptr));
    SeedHardwareFast(hasher);
    SeedTimestamp(hasher);
}
static void SeedSlow(CSHA512& hasher, RNGState& rng) noexcept
{
    unsigned char buffer[32];
    SeedFast(hasher);
    GetOSRand(buffer);
    hasher.Write(buffer, sizeof(buffer));
    rng.SeedEvents(hasher);
    SeedTimestamp(hasher);
}
static void SeedStrengthen(CSHA512& hasher, RNGState& rng, int microseconds) noexcept
{
    unsigned char strengthen_seed[32];
    rng.MixExtract(strengthen_seed, sizeof(strengthen_seed), CSHA512(hasher), false);
    Strengthen(strengthen_seed, microseconds, hasher);
}
static void SeedPeriodic(CSHA512& hasher, RNGState& rng) noexcept
{
    SeedFast(hasher);
    SeedTimestamp(hasher);
    rng.SeedEvents(hasher);
    auto old_size = hasher.Size();
    RandAddDynamicEnv(hasher);
    LogPrint(BCLog::RAND, "Feeding %i bytes of dynamic environment data into RNG\n", hasher.Size() - old_size);
    SeedStrengthen(hasher, rng, 10000);
}
static void SeedStartup(CSHA512& hasher, RNGState& rng) noexcept
{
    SeedHardwareSlow(hasher);
    SeedSlow(hasher, rng);
    auto old_size = hasher.Size();
    RandAddDynamicEnv(hasher);
    RandAddStaticEnv(hasher);
    LogPrint(BCLog::RAND, "Feeding %i bytes of environment data into RNG\n", hasher.Size() - old_size);
    SeedStrengthen(hasher, rng, 100000);
}
enum class RNGLevel {
    FAST,
    SLOW,
    PERIODIC,
};
static void ProcRand(unsigned char* out, int num, RNGLevel level) noexcept
{
    RNGState& rng = GetRNGState();
    assert(num <= 32);
    CSHA512 hasher;
    switch (level) {
    case RNGLevel::FAST:
        SeedFast(hasher);
        break;
    case RNGLevel::SLOW:
        SeedSlow(hasher, rng);
        break;
    case RNGLevel::PERIODIC:
        SeedPeriodic(hasher, rng);
        break;
    }
    if (!rng.MixExtract(out, num, std::move(hasher), false)) {
        CSHA512 startup_hasher;
        SeedStartup(startup_hasher, rng);
        rng.MixExtract(out, num, std::move(startup_hasher), true);
    }
}
void GetRandBytes(Span<unsigned char> bytes) noexcept { ProcRand(bytes.data(), bytes.size(), RNGLevel::FAST); }
void GetStrongRandBytes(Span<unsigned char> bytes) noexcept { ProcRand(bytes.data(), bytes.size(), RNGLevel::SLOW); }
void RandAddPeriodic() noexcept { ProcRand(nullptr, 0, RNGLevel::PERIODIC); }
void RandAddEvent(const uint32_t event_info) noexcept { GetRNGState().AddEvent(event_info); }
bool g_mock_deterministic_tests{false};
uint64_t GetRandInternal(uint64_t nMax) noexcept
{
    return FastRandomContext(g_mock_deterministic_tests).randrange(nMax);
}
uint256 GetRandHash() noexcept
{
    uint256 hash;
    GetRandBytes(hash);
    return hash;
}
void FastRandomContext::RandomSeed()
{
    uint256 seed = GetRandHash();
    rng.SetKey(seed.begin(), 32);
    requires_seed = false;
}
uint256 FastRandomContext::rand256() noexcept
{
    if (bytebuf_size < 32) {
        FillByteBuffer();
    }
    uint256 ret;
    memcpy(ret.begin(), bytebuf + 64 - bytebuf_size, 32);
    bytebuf_size -= 32;
    return ret;
}
std::vector<unsigned char> FastRandomContext::randbytes(size_t len)
{
    if (requires_seed) RandomSeed();
    std::vector<unsigned char> ret(len);
    if (len > 0) {
        rng.Keystream(ret.data(), len);
    }
    return ret;
}
FastRandomContext::FastRandomContext(const uint256& seed) noexcept : requires_seed(false), bytebuf_size(0), bitbuf_size(0)
{
    rng.SetKey(seed.begin(), 32);
}
bool Random_SanityCheck()
{
    uint64_t start = GetPerformanceCounter();
    static const ssize_t MAX_TRIES = 1024;
    uint8_t data[NUM_OS_RANDOM_BYTES];
    bool overwritten[NUM_OS_RANDOM_BYTES] = {};
    int num_overwritten;
    int tries = 0;
    do {
        memset(data, 0, NUM_OS_RANDOM_BYTES);
        GetOSRand(data);
        for (int x=0; x < NUM_OS_RANDOM_BYTES; ++x) {
            overwritten[x] |= (data[x] != 0);
        }
        num_overwritten = 0;
        for (int x=0; x < NUM_OS_RANDOM_BYTES; ++x) {
            if (overwritten[x]) {
                num_overwritten += 1;
            }
        }
        tries += 1;
    } while (num_overwritten < NUM_OS_RANDOM_BYTES && tries < MAX_TRIES);
    if (num_overwritten != NUM_OS_RANDOM_BYTES) return false;
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    uint64_t stop = GetPerformanceCounter();
    if (stop == start) return false;
    CSHA512 to_add;
    to_add.Write((const unsigned char*)&start, sizeof(start));
    to_add.Write((const unsigned char*)&stop, sizeof(stop));
    GetRNGState().MixExtract(nullptr, 0, std::move(to_add), false);
    return true;
}
FastRandomContext::FastRandomContext(bool fDeterministic) noexcept : requires_seed(!fDeterministic), bytebuf_size(0), bitbuf_size(0)
{
    if (!fDeterministic) {
        return;
    }
    uint256 seed;
    rng.SetKey(seed.begin(), 32);
}
FastRandomContext& FastRandomContext::operator=(FastRandomContext&& from) noexcept
{
    requires_seed = from.requires_seed;
    rng = from.rng;
    std::copy(std::begin(from.bytebuf), std::end(from.bytebuf), std::begin(bytebuf));
    bytebuf_size = from.bytebuf_size;
    bitbuf = from.bitbuf;
    bitbuf_size = from.bitbuf_size;
    from.requires_seed = true;
    from.bytebuf_size = 0;
    from.bitbuf_size = 0;
    return *this;
}
void RandomInit()
{
    ProcRand(nullptr, 0, RNGLevel::FAST);
    ReportHardwareRand();
}
std::chrono::microseconds GetExponentialRand(std::chrono::microseconds now, std::chrono::seconds average_interval)
{
    double unscaled = -std::log1p(GetRand(uint64_t{1} << 48) * -0.0000000000000035527136788  );
    return now + std::chrono::duration_cast<std::chrono::microseconds>(unscaled * average_interval + 0.5us);
}
