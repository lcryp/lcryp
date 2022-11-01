#if defined(HAVE_CONFIG_H)
#include <config/lcryp-config.h>
#endif
#include <timedata.h>
#include <netaddress.h>
#include <node/interface_ui.h>
#include <sync.h>
#include <tinyformat.h>
#include <util/system.h>
#include <util/translation.h>
#include <warnings.h>
static GlobalMutex g_timeoffset_mutex;
static int64_t nTimeOffset GUARDED_BY(g_timeoffset_mutex) = 0;
int64_t GetTimeOffset()
{
    LOCK(g_timeoffset_mutex);
    return nTimeOffset;
}
NodeClock::time_point GetAdjustedTime()
{
    return NodeClock::now() + std::chrono::seconds{GetTimeOffset()};
}
#define LCRYP_TIMEDATA_MAX_SAMPLES 200
static std::set<CNetAddr> g_sources;
static CMedianFilter<int64_t> g_time_offsets{LCRYP_TIMEDATA_MAX_SAMPLES, 0};
static bool g_warning_emitted;
void AddTimeData(const CNetAddr& ip, int64_t nOffsetSample)
{
    LOCK(g_timeoffset_mutex);
    if (g_sources.size() == LCRYP_TIMEDATA_MAX_SAMPLES)
        return;
    if (!g_sources.insert(ip).second)
        return;
    g_time_offsets.input(nOffsetSample);
    LogPrint(BCLog::NET, "added time data, samples %d, offset %+d (%+d minutes)\n", g_time_offsets.size(), nOffsetSample, nOffsetSample / 60);
    if (g_time_offsets.size() >= 5 && g_time_offsets.size() % 2 == 1) {
        int64_t nMedian = g_time_offsets.median();
        std::vector<int64_t> vSorted = g_time_offsets.sorted();
        int64_t max_adjustment = std::max<int64_t>(0, gArgs.GetIntArg("-maxtimeadjustment", DEFAULT_MAX_TIME_ADJUSTMENT));
        if (nMedian >= -max_adjustment && nMedian <= max_adjustment) {
            nTimeOffset = nMedian;
        } else {
            nTimeOffset = 0;
            if (!g_warning_emitted) {
                bool fMatch = false;
                for (const int64_t nOffset : vSorted) {
                    if (nOffset != 0 && nOffset > -5 * 60 && nOffset < 5 * 60) fMatch = true;
                }
                if (!fMatch) {
                    g_warning_emitted = true;
                    bilingual_str strMessage = strprintf(_("Please check that your computer's date and time are correct! If your clock is wrong, %s will not work properly."), PACKAGE_NAME);
                    SetMiscWarning(strMessage);
                    uiInterface.ThreadSafeMessageBox(strMessage, "", CClientUIInterface::MSG_WARNING);
                }
            }
        }
        if (LogAcceptCategory(BCLog::NET, BCLog::Level::Debug)) {
            std::string log_message{"time data samples: "};
            for (const int64_t n : vSorted) {
                log_message += strprintf("%+d  ", n);
            }
            log_message += strprintf("|  median offset = %+d  (%+d minutes)", nTimeOffset, nTimeOffset / 60);
            LogPrint(BCLog::NET, "%s\n", log_message);
        }
    }
}
void TestOnlyResetTimeData()
{
    LOCK(g_timeoffset_mutex);
    nTimeOffset = 0;
    g_sources.clear();
    g_time_offsets = CMedianFilter<int64_t>{LCRYP_TIMEDATA_MAX_SAMPLES, 0};
    g_warning_emitted = false;
}
