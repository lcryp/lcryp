#ifndef LCRYP_CHECKQUEUE_H
#define LCRYP_CHECKQUEUE_H
#include <sync.h>
#include <tinyformat.h>
#include <util/syscall_sandbox.h>
#include <util/threadnames.h>
#include <algorithm>
#include <vector>
template <typename T>
class CCheckQueueControl;
template <typename T>
class CCheckQueue
{
private:
    Mutex m_mutex;
    std::condition_variable m_worker_cv;
    std::condition_variable m_master_cv;
    std::vector<T> queue GUARDED_BY(m_mutex);
    int nIdle GUARDED_BY(m_mutex){0};
    int nTotal GUARDED_BY(m_mutex){0};
    bool fAllOk GUARDED_BY(m_mutex){true};
    unsigned int nTodo GUARDED_BY(m_mutex){0};
    const unsigned int nBatchSize;
    std::vector<std::thread> m_worker_threads;
    bool m_request_stop GUARDED_BY(m_mutex){false};
    bool Loop(bool fMaster) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
    {
        std::condition_variable& cond = fMaster ? m_master_cv : m_worker_cv;
        std::vector<T> vChecks;
        vChecks.reserve(nBatchSize);
        unsigned int nNow = 0;
        bool fOk = true;
        do {
            {
                WAIT_LOCK(m_mutex, lock);
                if (nNow) {
                    fAllOk &= fOk;
                    nTodo -= nNow;
                    if (nTodo == 0 && !fMaster)
                        m_master_cv.notify_one();
                } else {
                    nTotal++;
                }
                while (queue.empty() && !m_request_stop) {
                    if (fMaster && nTodo == 0) {
                        nTotal--;
                        bool fRet = fAllOk;
                        fAllOk = true;
                        return fRet;
                    }
                    nIdle++;
                    cond.wait(lock);
                    nIdle--;
                }
                if (m_request_stop) {
                    return false;
                }
                nNow = std::max(1U, std::min(nBatchSize, (unsigned int)queue.size() / (nTotal + nIdle + 1)));
                vChecks.resize(nNow);
                for (unsigned int i = 0; i < nNow; i++) {
                    vChecks[i].swap(queue.back());
                    queue.pop_back();
                }
                fOk = fAllOk;
            }
            for (T& check : vChecks)
                if (fOk)
                    fOk = check();
            vChecks.clear();
        } while (true);
    }
public:
    Mutex m_control_mutex;
    explicit CCheckQueue(unsigned int nBatchSizeIn)
        : nBatchSize(nBatchSizeIn)
    {
    }
    void StartWorkerThreads(const int threads_num) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
    {
        {
            LOCK(m_mutex);
            nIdle = 0;
            nTotal = 0;
            fAllOk = true;
        }
        assert(m_worker_threads.empty());
        for (int n = 0; n < threads_num; ++n) {
            m_worker_threads.emplace_back([this, n]() {
                util::ThreadRename(strprintf("scriptch.%i", n));
                SetSyscallSandboxPolicy(SyscallSandboxPolicy::VALIDATION_SCRIPT_CHECK);
                Loop(false  );
            });
        }
    }
    bool Wait() EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
    {
        return Loop(true  );
    }
    void Add(std::vector<T>& vChecks) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
    {
        if (vChecks.empty()) {
            return;
        }
        {
            LOCK(m_mutex);
            for (T& check : vChecks) {
                queue.emplace_back();
                check.swap(queue.back());
            }
            nTodo += vChecks.size();
        }
        if (vChecks.size() == 1) {
            m_worker_cv.notify_one();
        } else {
            m_worker_cv.notify_all();
        }
    }
    void StopWorkerThreads() EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
    {
        WITH_LOCK(m_mutex, m_request_stop = true);
        m_worker_cv.notify_all();
        for (std::thread& t : m_worker_threads) {
            t.join();
        }
        m_worker_threads.clear();
        WITH_LOCK(m_mutex, m_request_stop = false);
    }
    ~CCheckQueue()
    {
        assert(m_worker_threads.empty());
    }
};
template <typename T>
class CCheckQueueControl
{
private:
    CCheckQueue<T> * const pqueue;
    bool fDone;
public:
    CCheckQueueControl() = delete;
    CCheckQueueControl(const CCheckQueueControl&) = delete;
    CCheckQueueControl& operator=(const CCheckQueueControl&) = delete;
    explicit CCheckQueueControl(CCheckQueue<T> * const pqueueIn) : pqueue(pqueueIn), fDone(false)
    {
        if (pqueue != nullptr) {
            ENTER_CRITICAL_SECTION(pqueue->m_control_mutex);
        }
    }
    bool Wait()
    {
        if (pqueue == nullptr)
            return true;
        bool fRet = pqueue->Wait();
        fDone = true;
        return fRet;
    }
    void Add(std::vector<T>& vChecks)
    {
        if (pqueue != nullptr)
            pqueue->Add(vChecks);
    }
    ~CCheckQueueControl()
    {
        if (!fDone)
            Wait();
        if (pqueue != nullptr) {
            LEAVE_CRITICAL_SECTION(pqueue->m_control_mutex);
        }
    }
};
#endif
