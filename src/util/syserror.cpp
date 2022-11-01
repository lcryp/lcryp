#if defined(HAVE_CONFIG_H)
#include <config/lcryp-config.h>
#endif
#include <tinyformat.h>
#include <util/syserror.h>
#include <cstring>
#include <string>
std::string SysErrorString(int err)
{
    char buf[1024];
    const char *s = nullptr;
#ifdef WIN32
    if (strerror_s(buf, sizeof(buf), err) == 0) s = buf;
#else
#ifdef STRERROR_R_CHAR_P
    s = strerror_r(err, buf, sizeof(buf));
#else
    if (strerror_r(err, buf, sizeof(buf)) == 0) s = buf;
#endif
#endif
    if (s != nullptr) {
        return strprintf("%s (%d)", s, err);
    } else {
        return strprintf("Unknown error (%d)", err);
    }
}
