#ifndef LCRYP_UTIL_TIME_H
#define LCRYP_UTIL_TIME_H
#include <compat/compat.h>
#include <chrono>
#include <cstdint>
#include <string>
using namespace std::chrono_literals;
struct NodeClock : public std::chrono::system_clock {
    using time_point = std::chrono::time_point<NodeClock>;
    static time_point now() noexcept;
    static std::time_t to_time_t(const time_point&) = delete;
    static time_point from_time_t(std::time_t) = delete;
};
using NodeSeconds = std::chrono::time_point<NodeClock, std::chrono::seconds>;
using SteadyClock = std::chrono::steady_clock;
using SteadySeconds = std::chrono::time_point<std::chrono::steady_clock, std::chrono::seconds>;
using SteadyMilliseconds = std::chrono::time_point<std::chrono::steady_clock, std::chrono::milliseconds>;
using SteadyMicroseconds = std::chrono::time_point<std::chrono::steady_clock, std::chrono::microseconds>;
void UninterruptibleSleep(const std::chrono::microseconds& n);
template <typename Dur1, typename Dur2>
constexpr auto Ticks(Dur2 d)
{
    return std::chrono::duration_cast<Dur1>(d).count();
}
template <typename Duration, typename Timepoint>
constexpr auto TicksSinceEpoch(Timepoint t)
{
    return Ticks<Duration>(t.time_since_epoch());
}
constexpr int64_t count_seconds(std::chrono::seconds t) { return t.count(); }
constexpr int64_t count_milliseconds(std::chrono::milliseconds t) { return t.count(); }
constexpr int64_t count_microseconds(std::chrono::microseconds t) { return t.count(); }
using HoursDouble = std::chrono::duration<double, std::chrono::hours::period>;
using SecondsDouble = std::chrono::duration<double, std::chrono::seconds::period>;
int64_t GetTime();
int64_t GetTimeMillis();
int64_t GetTimeMicros();
void SetMockTime(int64_t nMockTimeIn);
void SetMockTime(std::chrono::seconds mock_time_in);
std::chrono::seconds GetMockTime();
template <typename T>
T Now()
{
    return std::chrono::time_point_cast<typename T::duration>(T::clock::now());
}
template <typename T>
T GetTime()
{
    return Now<std::chrono::time_point<NodeClock, T>>().time_since_epoch();
}
std::string FormatISO8601DateTime(int64_t nTime);
std::string FormatISO8601Date(int64_t nTime);
int64_t ParseISO8601DateTime(const std::string& str);
struct timeval MillisToTimeval(int64_t nTimeout);
struct timeval MillisToTimeval(std::chrono::milliseconds ms);
bool ChronoSanityCheck();
#endif
