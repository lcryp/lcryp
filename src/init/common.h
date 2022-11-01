#ifndef LCRYP_INIT_COMMON_H
#define LCRYP_INIT_COMMON_H
class ArgsManager;
namespace init {
void AddLoggingArgs(ArgsManager& args);
void SetLoggingOptions(const ArgsManager& args);
void SetLoggingCategories(const ArgsManager& args);
void SetLoggingLevel(const ArgsManager& args);
bool StartLogging(const ArgsManager& args);
void LogPackageVersion();
}
#endif
