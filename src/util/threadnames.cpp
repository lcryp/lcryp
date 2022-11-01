#if defined(HAVE_CONFIG_H)
#include <config/lcryp-config.h>
#endif
#include <string>
#include <thread>
#include <utility>
#if (defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__DragonFly__))
#include <pthread.h>
#include <pthread_np.h>
#endif
#include <util/threadnames.h>
#ifdef HAVE_SYS_PRCTL_H
#include <sys/prctl.h>
#endif
static void SetThreadName(const char* name)
{
#if defined(PR_SET_NAME)
    ::prctl(PR_SET_NAME, name, 0, 0, 0);
#elif (defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__DragonFly__))
    pthread_set_name_np(pthread_self(), name);
#elif defined(MAC_OSX)
    pthread_setname_np(name);
#else
    (void)name;
#endif
}
#if defined(HAVE_THREAD_LOCAL)
static thread_local std::string g_thread_name;
const std::string& util::ThreadGetInternalName() { return g_thread_name; }
static void SetInternalName(std::string name) { g_thread_name = std::move(name); }
#else
static const std::string empty_string;
const std::string& util::ThreadGetInternalName() { return empty_string; }
static void SetInternalName(std::string name) { }
#endif
void util::ThreadRename(std::string&& name)
{
    SetThreadName(("b-" + name).c_str());
    SetInternalName(std::move(name));
}
void util::ThreadSetInternalName(std::string&& name)
{
    SetInternalName(std::move(name));
}
