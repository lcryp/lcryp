#include <shutdown.h>
#if defined(HAVE_CONFIG_H)
#include <config/lcryp-config.h>
#endif
#include <logging.h>
#include <node/interface_ui.h>
#include <util/tokenpipe.h>
#include <warnings.h>
#include <assert.h>
#include <atomic>
#ifdef WIN32
#include <condition_variable>
#endif
bool AbortNode(const std::string& strMessage, bilingual_str user_message)
{
    SetMiscWarning(Untranslated(strMessage));
    LogPrintf("*** %s\n", strMessage);
    if (user_message.empty()) {
        user_message = _("A fatal internal error occurred, see debug.log for details");
    }
    AbortError(user_message);
    StartShutdown();
    return false;
}
static std::atomic<bool> fRequestShutdown(false);
#ifdef WIN32
std::mutex g_shutdown_mutex;
std::condition_variable g_shutdown_cv;
#else
static TokenPipeEnd g_shutdown_r;
static TokenPipeEnd g_shutdown_w;
#endif
bool InitShutdownState()
{
#ifndef WIN32
    std::optional<TokenPipe> pipe = TokenPipe::Make();
    if (!pipe) return false;
    g_shutdown_r = pipe->TakeReadEnd();
    g_shutdown_w = pipe->TakeWriteEnd();
#endif
    return true;
}
void StartShutdown()
{
#ifdef WIN32
    std::unique_lock<std::mutex> lk(g_shutdown_mutex);
    fRequestShutdown = true;
    g_shutdown_cv.notify_one();
#else
    if (!fRequestShutdown.exchange(true)) {
        int res = g_shutdown_w.TokenWrite('x');
        if (res != 0) {
            LogPrintf("Sending shutdown token failed\n");
            assert(0);
        }
    }
#endif
}
void AbortShutdown()
{
    if (fRequestShutdown) {
        WaitForShutdown();
    }
    fRequestShutdown = false;
}
bool ShutdownRequested()
{
    return fRequestShutdown;
}
void WaitForShutdown()
{
#ifdef WIN32
    std::unique_lock<std::mutex> lk(g_shutdown_mutex);
    g_shutdown_cv.wait(lk, [] { return fRequestShutdown.load(); });
#else
    int res = g_shutdown_r.TokenRead();
    if (res != 'x') {
        LogPrintf("Reading shutdown token failed\n");
        assert(0);
    }
#endif
}
