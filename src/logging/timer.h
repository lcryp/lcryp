#ifndef LCRYP_LOGGING_TIMER_H
#define LCRYP_LOGGING_TIMER_H
#include <logging.h>
#include <util/macros.h>
#include <util/time.h>
#include <util/types.h>
#include <chrono>
#include <string>
namespace BCLog {
template <typename TimeType>
class Timer
{
public:
    Timer(
        std::string prefix,
        std::string end_msg,
        BCLog::LogFlags log_category = BCLog::LogFlags::ALL,
        bool msg_on_completion = true) :
            m_prefix(std::move(prefix)),
            m_title(std::move(end_msg)),
            m_log_category(log_category),
            m_message_on_completion(msg_on_completion)
    {
        this->Log(strprintf("%s started", m_title));
        m_start_t = GetTime<std::chrono::microseconds>();
    }
    ~Timer()
    {
        if (m_message_on_completion) {
            this->Log(strprintf("%s completed", m_title));
        } else {
            this->Log("completed");
        }
    }
    void Log(const std::string& msg)
    {
        const std::string full_msg = this->LogMsg(msg);
        if (m_log_category == BCLog::LogFlags::ALL) {
            LogPrintf("%s\n", full_msg);
        } else {
            LogPrint(m_log_category, "%s\n", full_msg);
        }
    }
    std::string LogMsg(const std::string& msg)
    {
        const auto end_time = GetTime<std::chrono::microseconds>() - m_start_t;
        if (m_start_t.count() <= 0) {
            return strprintf("%s: %s", m_prefix, msg);
        }
        if constexpr (std::is_same<TimeType, std::chrono::microseconds>::value) {
            return strprintf("%s: %s (%iμs)", m_prefix, msg, end_time.count());
        } else if constexpr (std::is_same<TimeType, std::chrono::milliseconds>::value) {
            return strprintf("%s: %s (%.2fms)", m_prefix, msg, end_time.count() * 0.001);
        } else if constexpr (std::is_same<TimeType, std::chrono::seconds>::value) {
            return strprintf("%s: %s (%.2fs)", m_prefix, msg, end_time.count() * 0.000001);
        } else {
            static_assert(ALWAYS_FALSE<TimeType>, "Error: unexpected time type");
        }
    }
private:
    std::chrono::microseconds m_start_t{};
    const std::string m_prefix;
    const std::string m_title;
    const BCLog::LogFlags m_log_category;
    const bool m_message_on_completion;
};
}
#define LOG_TIME_MICROS_WITH_CATEGORY(end_msg, log_category) \
    BCLog::Timer<std::chrono::microseconds> UNIQUE_NAME(logging_timer)(__func__, end_msg, log_category)
#define LOG_TIME_MILLIS_WITH_CATEGORY(end_msg, log_category) \
    BCLog::Timer<std::chrono::milliseconds> UNIQUE_NAME(logging_timer)(__func__, end_msg, log_category)
#define LOG_TIME_MILLIS_WITH_CATEGORY_MSG_ONCE(end_msg, log_category) \
    BCLog::Timer<std::chrono::milliseconds> UNIQUE_NAME(logging_timer)(__func__, end_msg, log_category,  false)
#define LOG_TIME_SECONDS(end_msg) \
    BCLog::Timer<std::chrono::seconds> UNIQUE_NAME(logging_timer)(__func__, end_msg)
#endif
