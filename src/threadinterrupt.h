#ifndef LCRYP_THREADINTERRUPT_H
#define LCRYP_THREADINTERRUPT_H
#include <sync.h>
#include <threadsafety.h>
#include <atomic>
#include <chrono>
#include <condition_variable>
class CThreadInterrupt
{
public:
    using Clock = std::chrono::steady_clock;
    CThreadInterrupt();
    explicit operator bool() const;
    void operator()() EXCLUSIVE_LOCKS_REQUIRED(!mut);
    void reset();
    bool sleep_for(Clock::duration rel_time) EXCLUSIVE_LOCKS_REQUIRED(!mut);
private:
    std::condition_variable cond;
    Mutex mut;
    std::atomic<bool> flag;
};
#endif
