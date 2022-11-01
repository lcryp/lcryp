#include <threadinterrupt.h>
#include <sync.h>
CThreadInterrupt::CThreadInterrupt() : flag(false) {}
CThreadInterrupt::operator bool() const
{
    return flag.load(std::memory_order_acquire);
}
void CThreadInterrupt::reset()
{
    flag.store(false, std::memory_order_release);
}
void CThreadInterrupt::operator()()
{
    {
        LOCK(mut);
        flag.store(true, std::memory_order_release);
    }
    cond.notify_all();
}
bool CThreadInterrupt::sleep_for(Clock::duration rel_time)
{
    WAIT_LOCK(mut, lock);
    return !cond.wait_for(lock, rel_time, [this]() { return flag.load(std::memory_order_acquire); });
}
