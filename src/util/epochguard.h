#ifndef LCRYP_UTIL_EPOCHGUARD_H
#define LCRYP_UTIL_EPOCHGUARD_H
#include <threadsafety.h>
#include <util/macros.h>
#include <cassert>
class LOCKABLE Epoch
{
private:
    uint64_t m_raw_epoch = 0;
    bool m_guarded = false;
public:
    Epoch() = default;
    Epoch(const Epoch&) = delete;
    Epoch& operator=(const Epoch&) = delete;
    Epoch(Epoch&&) = delete;
    Epoch& operator=(Epoch&&) = delete;
    ~Epoch() = default;
    bool guarded() const { return m_guarded; }
    class Marker
    {
    private:
        uint64_t m_marker = 0;
        friend class Epoch;
        Marker& operator=(const Marker&) = delete;
    public:
        Marker() = default;
        Marker(const Marker&) = default;
        Marker(Marker&&) = delete;
        Marker& operator=(Marker&&) = delete;
        ~Marker() = default;
    };
    class SCOPED_LOCKABLE Guard
    {
    private:
        Epoch& m_epoch;
    public:
        explicit Guard(Epoch& epoch) EXCLUSIVE_LOCK_FUNCTION(epoch) : m_epoch(epoch)
        {
            assert(!m_epoch.m_guarded);
            ++m_epoch.m_raw_epoch;
            m_epoch.m_guarded = true;
        }
        ~Guard() UNLOCK_FUNCTION()
        {
            assert(m_epoch.m_guarded);
            ++m_epoch.m_raw_epoch;
            m_epoch.m_guarded = false;
        }
    };
    bool visited(Marker& marker) const EXCLUSIVE_LOCKS_REQUIRED(*this)
    {
        assert(m_guarded);
        if (marker.m_marker < m_raw_epoch) {
            marker.m_marker = m_raw_epoch;
            return false;
        } else {
            return true;
        }
    }
};
#define WITH_FRESH_EPOCH(epoch) const Epoch::Guard UNIQUE_NAME(epoch_guard_)(epoch)
#endif
