#include <scheduler.h>
#include <sync.h>
#include <util/syscall_sandbox.h>
#include <util/time.h>
#include <cassert>
#include <functional>
#include <utility>
CScheduler::CScheduler() = default;
CScheduler::~CScheduler()
{
    assert(nThreadsServicingQueue == 0);
    if (stopWhenEmpty) assert(taskQueue.empty());
}
void CScheduler::serviceQueue()
{
    SetSyscallSandboxPolicy(SyscallSandboxPolicy::SCHEDULER);
    WAIT_LOCK(newTaskMutex, lock);
    ++nThreadsServicingQueue;
    while (!shouldStop()) {
        try {
            while (!shouldStop() && taskQueue.empty()) {
                newTaskScheduled.wait(lock);
            }
            while (!shouldStop() && !taskQueue.empty()) {
                std::chrono::steady_clock::time_point timeToWaitFor = taskQueue.begin()->first;
                if (newTaskScheduled.wait_until(lock, timeToWaitFor) == std::cv_status::timeout) {
                    break;
                }
            }
            if (shouldStop() || taskQueue.empty())
                continue;
            Function f = taskQueue.begin()->second;
            taskQueue.erase(taskQueue.begin());
            {
                REVERSE_LOCK(lock);
                f();
            }
        } catch (...) {
            --nThreadsServicingQueue;
            throw;
        }
    }
    --nThreadsServicingQueue;
    newTaskScheduled.notify_one();
}
void CScheduler::schedule(CScheduler::Function f, std::chrono::steady_clock::time_point t)
{
    {
        LOCK(newTaskMutex);
        taskQueue.insert(std::make_pair(t, f));
    }
    newTaskScheduled.notify_one();
}
void CScheduler::MockForward(std::chrono::seconds delta_seconds)
{
    assert(delta_seconds > 0s && delta_seconds <= 1h);
    {
        LOCK(newTaskMutex);
        std::multimap<std::chrono::steady_clock::time_point, Function> temp_queue;
        for (const auto& element : taskQueue) {
            temp_queue.emplace_hint(temp_queue.cend(), element.first - delta_seconds, element.second);
        }
        taskQueue = std::move(temp_queue);
    }
    newTaskScheduled.notify_one();
}
static void Repeat(CScheduler& s, CScheduler::Function f, std::chrono::milliseconds delta)
{
    f();
    s.scheduleFromNow([=, &s] { Repeat(s, f, delta); }, delta);
}
void CScheduler::scheduleEvery(CScheduler::Function f, std::chrono::milliseconds delta)
{
    scheduleFromNow([this, f, delta] { Repeat(*this, f, delta); }, delta);
}
size_t CScheduler::getQueueInfo(std::chrono::steady_clock::time_point& first,
                                std::chrono::steady_clock::time_point& last) const
{
    LOCK(newTaskMutex);
    size_t result = taskQueue.size();
    if (!taskQueue.empty()) {
        first = taskQueue.begin()->first;
        last = taskQueue.rbegin()->first;
    }
    return result;
}
bool CScheduler::AreThreadsServicingQueue() const
{
    LOCK(newTaskMutex);
    return nThreadsServicingQueue;
}
void SingleThreadedSchedulerClient::MaybeScheduleProcessQueue()
{
    {
        LOCK(m_callbacks_mutex);
        if (m_are_callbacks_running) return;
        if (m_callbacks_pending.empty()) return;
    }
    m_scheduler.schedule([this] { this->ProcessQueue(); }, std::chrono::steady_clock::now());
}
void SingleThreadedSchedulerClient::ProcessQueue()
{
    std::function<void()> callback;
    {
        LOCK(m_callbacks_mutex);
        if (m_are_callbacks_running) return;
        if (m_callbacks_pending.empty()) return;
        m_are_callbacks_running = true;
        callback = std::move(m_callbacks_pending.front());
        m_callbacks_pending.pop_front();
    }
    struct RAIICallbacksRunning {
        SingleThreadedSchedulerClient* instance;
        explicit RAIICallbacksRunning(SingleThreadedSchedulerClient* _instance) : instance(_instance) {}
        ~RAIICallbacksRunning()
        {
            {
                LOCK(instance->m_callbacks_mutex);
                instance->m_are_callbacks_running = false;
            }
            instance->MaybeScheduleProcessQueue();
        }
    } raiicallbacksrunning(this);
    callback();
}
void SingleThreadedSchedulerClient::AddToProcessQueue(std::function<void()> func)
{
    {
        LOCK(m_callbacks_mutex);
        m_callbacks_pending.emplace_back(std::move(func));
    }
    MaybeScheduleProcessQueue();
}
void SingleThreadedSchedulerClient::EmptyQueue()
{
    assert(!m_scheduler.AreThreadsServicingQueue());
    bool should_continue = true;
    while (should_continue) {
        ProcessQueue();
        LOCK(m_callbacks_mutex);
        should_continue = !m_callbacks_pending.empty();
    }
}
size_t SingleThreadedSchedulerClient::CallbacksPending()
{
    LOCK(m_callbacks_mutex);
    return m_callbacks_pending.size();
}
