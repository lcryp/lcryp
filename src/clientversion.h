#ifndef LCRYP_CLIENTVERSION_H
#define LCRYP_CLIENTVERSION_H
#include <util/macros.h>
#if defined(HAVE_CONFIG_H)
#include <config/lcryp-config.h>
#endif
#if !defined(CLIENT_VERSION_MAJOR) || !defined(CLIENT_VERSION_MINOR) || !defined(CLIENT_VERSION_BUILD) || !defined(CLIENT_VERSION_IS_RELEASE) || !defined(COPYRIGHT_YEAR)
#error Client version information missing: version is not defined by lcryp-config.h or in any other way
#endif
#define COPYRIGHT_STR "2022-" STRINGIZE(COPYRIGHT_YEAR) " " COPYRIGHT_HOLDERS_FINAL
#if !defined(WINDRES_PREPROC)
#include <string>
#include <vector>
static const int CLIENT_VERSION =
                             10000 * CLIENT_VERSION_MAJOR
                         +     100 * CLIENT_VERSION_MINOR
                         +       1 * CLIENT_VERSION_BUILD;
extern const std::string CLIENT_NAME;
std::string FormatFullVersion();
std::string FormatSubVersion(const std::string& name, int nClientVersion, const std::vector<std::string>& comments);
std::string CopyrightHolders(const std::string& strPrefix);
std::string LicenseInfo();
#endif
#endif
