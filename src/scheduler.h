#ifndef LCRYP_SCHEDULER_H
#define LCRYP_SCHEDULER_H
#include <attributes.h>
#include <sync.h>
#include <threadsafety.h>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <list>
#include <map>
#include <thread>
#include <utility>
class CScheduler
{
public:
    CScheduler();
    ~CScheduler();
    std::thread m_service_thread;
    typedef std::function<void()> Function;
    void schedule(Function f, std::chrono::steady_clock::time_point t) EXCLUSIVE_LOCKS_REQUIRED(!newTaskMutex);
    void scheduleFromNow(Function f, std::chrono::milliseconds delta) EXCLUSIVE_LOCKS_REQUIRED(!newTaskMutex)
    {
        schedule(std::move(f), std::chrono::steady_clock::now() + delta);
    }
    void scheduleEvery(Function f, std::chrono::milliseconds delta) EXCLUSIVE_LOCKS_REQUIRED(!newTaskMutex);
    void MockForward(std::chrono::seconds delta_seconds) EXCLUSIVE_LOCKS_REQUIRED(!newTaskMutex);
    void serviceQueue() EXCLUSIVE_LOCKS_REQUIRED(!newTaskMutex);
    void stop() EXCLUSIVE_LOCKS_REQUIRED(!newTaskMutex)
    {
        WITH_LOCK(newTaskMutex, stopRequested = true);
        newTaskScheduled.notify_all();
        if (m_service_thread.joinable()) m_service_thread.join();
    }
    void StopWhenDrained() EXCLUSIVE_LOCKS_REQUIRED(!newTaskMutex)
    {
        WITH_LOCK(newTaskMutex, stopWhenEmpty = true);
        newTaskScheduled.notify_all();
        if (m_service_thread.joinable()) m_service_thread.join();
    }
    size_t getQueueInfo(std::chrono::steady_clock::time_point& first,
                        std::chrono::steady_clock::time_point& last) const
        EXCLUSIVE_LOCKS_REQUIRED(!newTaskMutex);
    bool AreThreadsServicingQueue() const EXCLUSIVE_LOCKS_REQUIRED(!newTaskMutex);
private:
    mutable Mutex newTaskMutex;
    std::condition_variable newTaskScheduled;
    std::multimap<std::chrono::steady_clock::time_point, Function> taskQueue GUARDED_BY(newTaskMutex);
    int nThreadsServicingQueue GUARDED_BY(newTaskMutex){0};
    bool stopRequested GUARDED_BY(newTaskMutex){false};
    bool stopWhenEmpty GUARDED_BY(newTaskMutex){false};
    bool shouldStop() const EXCLUSIVE_LOCKS_REQUIRED(newTaskMutex) { return stopRequested || (stopWhenEmpty && taskQueue.empty()); }
};
class SingleThreadedSchedulerClient
{
private:
    CScheduler& m_scheduler;
    Mutex m_callbacks_mutex;
    std::list<std::function<void()>> m_callbacks_pending GUARDED_BY(m_callbacks_mutex);
    bool m_are_callbacks_running GUARDED_BY(m_callbacks_mutex) = false;
    void MaybeScheduleProcessQueue() EXCLUSIVE_LOCKS_REQUIRED(!m_callbacks_mutex);
    void ProcessQueue() EXCLUSIVE_LOCKS_REQUIRED(!m_callbacks_mutex);
public:
    explicit SingleThreadedSchedulerClient(CScheduler& scheduler LIFETIMEBOUND) : m_scheduler{scheduler} {}
    void AddToProcessQueue(std::function<void()> func) EXCLUSIVE_LOCKS_REQUIRED(!m_callbacks_mutex);
    void EmptyQueue() EXCLUSIVE_LOCKS_REQUIRED(!m_callbacks_mutex);
    size_t CallbacksPending() EXCLUSIVE_LOCKS_REQUIRED(!m_callbacks_mutex);
};
#endif
