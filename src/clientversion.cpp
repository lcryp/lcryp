#include <clientversion.h>
#include <util/translation.h>
#include <tinyformat.h>
#include <sstream>
#include <string>
#include <vector>
const std::string CLIENT_NAME("Ryp");
#ifdef HAVE_BUILD_INFO
#include <obj/build.h>
#endif
#define GIT_COMMIT_ID "437b608df289c97fd88f1dd79bdc8359e1b1c5b1"
#ifdef BUILD_GIT_TAG
    #define BUILD_DESC BUILD_GIT_TAG
    #define BUILD_SUFFIX ""
#else
    #define BUILD_DESC "v" PACKAGE_VERSION
    #if CLIENT_VERSION_IS_RELEASE
        #define BUILD_SUFFIX ""
    #elif defined(BUILD_GIT_COMMIT)
        #define BUILD_SUFFIX "-" BUILD_GIT_COMMIT
    #elif defined(GIT_COMMIT_ID)
        #define BUILD_SUFFIX "-g" GIT_COMMIT_ID
    #else
        #define BUILD_SUFFIX "-unk"
    #endif
#endif
static std::string FormatVersion(int nVersion)
{
    return strprintf("%d.%d.%d", nVersion / 10000, (nVersion / 100) % 100, nVersion % 100);
}
std::string FormatFullVersion()
{
    static const std::string CLIENT_BUILD(BUILD_DESC BUILD_SUFFIX);
    return CLIENT_BUILD;
}
std::string FormatSubVersion(const std::string& name, int nClientVersion, const std::vector<std::string>& comments)
{
    std::ostringstream ss;
    ss << "/";
    ss << name << ":" << FormatVersion(nClientVersion);
    if (!comments.empty())
    {
        std::vector<std::string>::const_iterator it(comments.begin());
        ss << "(" << *it;
        for(++it; it != comments.end(); ++it)
            ss << "; " << *it;
        ss << ")";
    }
    ss << "/";
    return ss.str();
}
std::string CopyrightHolders(const std::string& strPrefix)
{
    const auto copyright_devs = strprintf(_(COPYRIGHT_HOLDERS).translated, COPYRIGHT_HOLDERS_SUBSTITUTION);
    std::string strCopyrightHolders = strPrefix + copyright_devs;
    if (copyright_devs.find("LcRyp Core") == std::string::npos) {
        strCopyrightHolders += "\n" + strPrefix + "The LcRyp Core developers";
    }
    return strCopyrightHolders;
}
std::string LicenseInfo()
{
    const std::string URL_SOURCE_CODE = "<https://github.com/lcryp/lcryp>";
    return CopyrightHolders(strprintf(_("Copyright (C) %i-%i").translated, 2022, COPYRIGHT_YEAR) + " ") + "\n" +
           "\n" +
           strprintf(_("Please contribute if you find %s useful. "
                       "Visit %s for further information about the software.").translated, PACKAGE_NAME, "<" PACKAGE_URL ">") +
           "\n" +
           strprintf(_("The source code is available from %s.").translated, URL_SOURCE_CODE) +
           "\n" +
           "\n" +
           _("This is experimental software.").translated + "\n" +
           strprintf(_("Distributed under the MIT software license, see the accompanying file %s or %s").translated, "COPYING", "<https://opensource.org/licenses/MIT>") +
           "\n";
}
