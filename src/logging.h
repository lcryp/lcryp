#ifndef LCRYP_LOGGING_H
#define LCRYP_LOGGING_H
#include <fs.h>
#include <threadsafety.h>
#include <tinyformat.h>
#include <util/string.h>
#include <atomic>
#include <cstdint>
#include <functional>
#include <list>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>
static const bool DEFAULT_LOGTIMEMICROS = false;
static const bool DEFAULT_LOGIPS        = false;
static const bool DEFAULT_LOGTIMESTAMPS = true;
static const bool DEFAULT_LOGTHREADNAMES = false;
static const bool DEFAULT_LOGSOURCELOCATIONS = false;
extern const char * const DEFAULT_DEBUGLOGFILE;
extern bool fLogIPs;
struct LogCategory {
    std::string category;
    bool active;
};
namespace BCLog {
    enum LogFlags : uint32_t {
        NONE        = 0,
        NET         = (1 <<  0),
        TOR         = (1 <<  1),
        MEMPOOL     = (1 <<  2),
        HTTP        = (1 <<  3),
        BENCH       = (1 <<  4),
        ZMQ         = (1 <<  5),
        WALLETDB    = (1 <<  6),
        RPC         = (1 <<  7),
        ESTIMATEFEE = (1 <<  8),
        ADDRMAN     = (1 <<  9),
        SELECTCOINS = (1 << 10),
        REINDEX     = (1 << 11),
        CMPCTBLOCK  = (1 << 12),
        RAND        = (1 << 13),
        PRUNE       = (1 << 14),
        PROXY       = (1 << 15),
        MEMPOOLREJ  = (1 << 16),
        LIBEVENT    = (1 << 17),
        COINDB      = (1 << 18),
        QT          = (1 << 19),
        LEVELDB     = (1 << 20),
        VALIDATION  = (1 << 21),
        I2P         = (1 << 22),
        IPC         = (1 << 23),
#ifdef DEBUG_LOCKCONTENTION
        LOCK        = (1 << 24),
#endif
        UTIL        = (1 << 25),
        BLOCKSTORE  = (1 << 26),
        MINING      = (1 << 27),
        ALL         = ~(uint32_t)0,
    };
    enum class Level {
        Trace = 0,
        Debug,
        Info,
        Warning,
        Error,
        None,
    };
    constexpr auto DEFAULT_LOG_LEVEL{Level::Debug};
    class Logger
    {
    private:
        mutable StdMutex m_cs;
        FILE* m_fileout GUARDED_BY(m_cs) = nullptr;
        std::list<std::string> m_msgs_before_open GUARDED_BY(m_cs);
        bool m_buffering GUARDED_BY(m_cs) = true;
        std::atomic_bool m_started_new_line{true};
        std::unordered_map<LogFlags, Level> m_category_log_levels GUARDED_BY(m_cs);
        std::atomic<Level> m_log_level{DEFAULT_LOG_LEVEL};
        std::atomic<uint32_t> m_categories{0};
        std::string LogTimestampStr(const std::string& str);
        std::list<std::function<void(const std::string&)>> m_print_callbacks GUARDED_BY(m_cs) {};
    public:
        bool m_print_to_console = false;
        bool m_print_to_file = false;
        bool m_log_timestamps = DEFAULT_LOGTIMESTAMPS;
        bool m_log_time_micros = DEFAULT_LOGTIMEMICROS;
        bool m_log_threadnames = DEFAULT_LOGTHREADNAMES;
        bool m_log_sourcelocations = DEFAULT_LOGSOURCELOCATIONS;
        fs::path m_file_path;
        std::atomic<bool> m_reopen_file{false};
        void LogPrintStr(const std::string& str, const std::string& logging_function, const std::string& source_file, int source_line, BCLog::LogFlags category, BCLog::Level level);
        bool Enabled() const
        {
            StdLockGuard scoped_lock(m_cs);
            return m_buffering || m_print_to_console || m_print_to_file || !m_print_callbacks.empty();
        }
        std::list<std::function<void(const std::string&)>>::iterator PushBackCallback(std::function<void(const std::string&)> fun)
        {
            StdLockGuard scoped_lock(m_cs);
            m_print_callbacks.push_back(std::move(fun));
            return --m_print_callbacks.end();
        }
        void DeleteCallback(std::list<std::function<void(const std::string&)>>::iterator it)
        {
            StdLockGuard scoped_lock(m_cs);
            m_print_callbacks.erase(it);
        }
        bool StartLogging();
        void DisconnectTestLogger();
        void ShrinkDebugFile();
        std::unordered_map<LogFlags, Level> CategoryLevels() const
        {
            StdLockGuard scoped_lock(m_cs);
            return m_category_log_levels;
        }
        void SetCategoryLogLevel(const std::unordered_map<LogFlags, Level>& levels)
        {
            StdLockGuard scoped_lock(m_cs);
            m_category_log_levels = levels;
        }
        bool SetCategoryLogLevel(const std::string& category_str, const std::string& level_str);
        Level LogLevel() const { return m_log_level.load(); }
        void SetLogLevel(Level level) { m_log_level = level; }
        bool SetLogLevel(const std::string& level);
        uint32_t GetCategoryMask() const { return m_categories.load(); }
        void EnableCategory(LogFlags flag);
        bool EnableCategory(const std::string& str);
        void DisableCategory(LogFlags flag);
        bool DisableCategory(const std::string& str);
        bool WillLogCategory(LogFlags category) const;
        bool WillLogCategoryLevel(LogFlags category, Level level) const;
        std::vector<LogCategory> LogCategoriesList() const;
        std::string LogCategoriesString() const
        {
            return Join(LogCategoriesList(), ", ", [&](const LogCategory& i) { return i.category; });
        };
        std::string LogLevelsString() const;
        std::string LogLevelToStr(BCLog::Level level) const;
        bool DefaultShrinkDebugFile() const;
    };
}
BCLog::Logger& LogInstance();
static inline bool LogAcceptCategory(BCLog::LogFlags category, BCLog::Level level)
{
    return LogInstance().WillLogCategoryLevel(category, level);
}
bool GetLogCategory(BCLog::LogFlags& flag, const std::string& str);
template <typename... Args>
static inline void LogPrintf_(const std::string& logging_function, const std::string& source_file, const int source_line, const BCLog::LogFlags flag, const BCLog::Level level, const char* fmt, const Args&... args)
{
    if (LogInstance().Enabled()) {
        std::string log_msg;
        try {
            log_msg = tfm::format(fmt, args...);
        } catch (tinyformat::format_error& fmterr) {
            log_msg = "Error \"" + std::string(fmterr.what()) + "\" while formatting log message: " + fmt;
        }
        LogInstance().LogPrintStr(log_msg, logging_function, source_file, source_line, flag, level);
    }
}
#define LogPrintLevel_(category, level, ...) LogPrintf_(__func__, __FILE__, __LINE__, category, level, __VA_ARGS__)
#define LogPrintf(...) LogPrintLevel_(BCLog::LogFlags::NONE, BCLog::Level::None, __VA_ARGS__)
#define LogPrintfCategory(category, ...) LogPrintLevel_(category, BCLog::Level::None, __VA_ARGS__)
#define LogPrint(category, ...)                                        \
    do {                                                               \
        if (LogAcceptCategory((category), BCLog::Level::Debug)) {      \
            LogPrintLevel_(category, BCLog::Level::None, __VA_ARGS__); \
        }                                                              \
    } while (0)
#define LogPrintLevel(category, level, ...)               \
    do {                                                  \
        if (LogAcceptCategory((category), (level))) {     \
            LogPrintLevel_(category, level, __VA_ARGS__); \
        }                                                 \
    } while (0)
#endif
