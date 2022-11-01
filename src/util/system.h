#ifndef LCRYP_UTIL_SYSTEM_H
#define LCRYP_UTIL_SYSTEM_H
#if defined(HAVE_CONFIG_H)
#include <config/lcryp-config.h>
#endif
#include <compat/compat.h>
#include <compat/assumptions.h>
#include <fs.h>
#include <logging.h>
#include <sync.h>
#include <tinyformat.h>
#include <util/settings.h>
#include <util/time.h>
#include <any>
#include <exception>
#include <map>
#include <optional>
#include <set>
#include <stdint.h>
#include <string>
#include <utility>
#include <vector>
class UniValue;
int64_t GetStartupTime();
extern const char * const LCRYP_CONF_FILENAME;
extern const char * const LCRYP_SETTINGS_FILENAME;
void SetupEnvironment();
bool SetupNetworking();
template<typename... Args>
bool error(const char* fmt, const Args&... args)
{
    LogPrintf("ERROR: %s\n", tfm::format(fmt, args...));
    return false;
}
void PrintExceptionContinue(const std::exception* pex, std::string_view thread_name);
bool FileCommit(FILE *file);
void DirectoryCommit(const fs::path &dirname);
bool TruncateFile(FILE *file, unsigned int length);
int RaiseFileDescriptorLimit(int nMinFD);
void AllocateFileRange(FILE *file, unsigned int offset, unsigned int length);
[[nodiscard]] bool RenameOver(fs::path src, fs::path dest);
bool LockDirectory(const fs::path& directory, const fs::path& lockfile_name, bool probe_only=false);
void UnlockDirectory(const fs::path& directory, const fs::path& lockfile_name);
bool DirIsWritable(const fs::path& directory);
bool CheckDiskSpace(const fs::path& dir, uint64_t additional_bytes = 0);
std::streampos GetFileSize(const char* path, std::streamsize max = std::numeric_limits<std::streamsize>::max());
void ReleaseDirectoryLocks();
bool TryCreateDirectories(const fs::path& p);
fs::path GetDefaultDataDir();
bool CheckDataDirOption();
fs::path GetConfigFile(const fs::path& configuration_file_path);
#ifdef WIN32
fs::path GetSpecialFolderPath(int nFolder, bool fCreate = true);
#endif
#ifndef WIN32
std::string ShellEscape(const std::string& arg);
#endif
#if HAVE_SYSTEM
void runCommand(const std::string& strCommand);
#endif
UniValue RunCommandParseJSON(const std::string& str_command, const std::string& str_std_in="");
fs::path AbsPathForConfigVal(const fs::path& path, bool net_specific = true);
inline bool IsSwitchChar(char c)
{
#ifdef WIN32
    return c == '-' || c == '/';
#else
    return c == '-';
#endif
}
enum class OptionsCategory {
    OPTIONS,
    MINING,
    CONNECTION,
    WALLET,
    WALLET_DEBUG_TEST,
    ZMQ,
    DEBUG_TEST,
    CHAINPARAMS,
    NODE_RELAY,
    BLOCK_CREATION,
    RPC,
    GUI,
    COMMANDS,
    REGISTER_COMMANDS,
    HIDDEN
};
struct SectionInfo
{
    std::string m_name;
    std::string m_file;
    int m_line;
};
std::string SettingToString(const util::SettingsValue&, const std::string&);
std::optional<std::string> SettingToString(const util::SettingsValue&);
int64_t SettingToInt(const util::SettingsValue&, int64_t);
std::optional<int64_t> SettingToInt(const util::SettingsValue&);
bool SettingToBool(const util::SettingsValue&, bool);
std::optional<bool> SettingToBool(const util::SettingsValue&);
class ArgsManager
{
public:
    enum Flags : uint32_t {
        ALLOW_ANY = 0x01,
        DISALLOW_NEGATION = 0x20,
        DISALLOW_ELISION = 0x40,
        DEBUG_ONLY = 0x100,
        NETWORK_ONLY = 0x200,
        SENSITIVE = 0x400,
        COMMAND = 0x800,
    };
protected:
    struct Arg
    {
        std::string m_help_param;
        std::string m_help_text;
        unsigned int m_flags;
    };
    mutable RecursiveMutex cs_args;
    util::Settings m_settings GUARDED_BY(cs_args);
    std::vector<std::string> m_command GUARDED_BY(cs_args);
    std::string m_network GUARDED_BY(cs_args);
    std::set<std::string> m_network_only_args GUARDED_BY(cs_args);
    std::map<OptionsCategory, std::map<std::string, Arg>> m_available_args GUARDED_BY(cs_args);
    bool m_accept_any_command GUARDED_BY(cs_args){true};
    std::list<SectionInfo> m_config_sections GUARDED_BY(cs_args);
    mutable fs::path m_cached_blocks_path GUARDED_BY(cs_args);
    mutable fs::path m_cached_datadir_path GUARDED_BY(cs_args);
    mutable fs::path m_cached_network_datadir_path GUARDED_BY(cs_args);
    [[nodiscard]] bool ReadConfigStream(std::istream& stream, const std::string& filepath, std::string& error, bool ignore_invalid_keys = false);
    bool UseDefaultSection(const std::string& arg) const EXCLUSIVE_LOCKS_REQUIRED(cs_args);
 public:
    util::SettingsValue GetSetting(const std::string& arg) const;
    std::vector<util::SettingsValue> GetSettingsList(const std::string& arg) const;
    ArgsManager();
    ~ArgsManager();
    void SelectConfigNetwork(const std::string& network);
    [[nodiscard]] bool ParseParameters(int argc, const char* const argv[], std::string& error);
    [[nodiscard]] bool ReadConfigFiles(std::string& error, bool ignore_invalid_keys = false);
    const std::set<std::string> GetUnsuitableSectionOnlyArgs() const;
    const std::list<SectionInfo> GetUnrecognizedSections() const;
    struct Command {
        std::string command;
        std::vector<std::string> args;
    };
    std::optional<const Command> GetCommand() const;
    const fs::path& GetBlocksDirPath() const;
    const fs::path& GetDataDirBase() const { return GetDataDir(false); }
    const fs::path& GetDataDirNet() const { return GetDataDir(true); }
    void ClearPathCache();
    std::vector<std::string> GetArgs(const std::string& strArg) const;
    bool IsArgSet(const std::string& strArg) const;
    bool IsArgNegated(const std::string& strArg) const;
    std::string GetArg(const std::string& strArg, const std::string& strDefault) const;
    std::optional<std::string> GetArg(const std::string& strArg) const;
    fs::path GetPathArg(std::string arg, const fs::path& default_value = {}) const;
    int64_t GetIntArg(const std::string& strArg, int64_t nDefault) const;
    std::optional<int64_t> GetIntArg(const std::string& strArg) const;
    bool GetBoolArg(const std::string& strArg, bool fDefault) const;
    std::optional<bool> GetBoolArg(const std::string& strArg) const;
    bool SoftSetArg(const std::string& strArg, const std::string& strValue);
    bool SoftSetBoolArg(const std::string& strArg, bool fValue);
    void ForceSetArg(const std::string& strArg, const std::string& strValue);
    std::string GetChainName() const;
    void AddArg(const std::string& name, const std::string& help, unsigned int flags, const OptionsCategory& cat);
    void AddCommand(const std::string& cmd, const std::string& help);
    void AddHiddenArgs(const std::vector<std::string>& args);
    void ClearArgs() {
        LOCK(cs_args);
        m_available_args.clear();
        m_network_only_args.clear();
    }
    std::string GetHelpMessage() const;
    std::optional<unsigned int> GetArgFlags(const std::string& name) const;
    bool InitSettings(std::string& error);
    bool GetSettingsPath(fs::path* filepath = nullptr, bool temp = false, bool backup = false) const;
    bool ReadSettingsFile(std::vector<std::string>* errors = nullptr);
    bool WriteSettingsFile(std::vector<std::string>* errors = nullptr, bool backup = false) const;
    util::SettingsValue GetPersistentSetting(const std::string& name) const;
    template <typename Fn>
    void LockSettings(Fn&& fn)
    {
        LOCK(cs_args);
        fn(m_settings);
    }
    void LogArgs() const;
private:
    const fs::path& GetDataDir(bool net_specific) const;
    void logArgsPrefix(
        const std::string& prefix,
        const std::string& section,
        const std::map<std::string, std::vector<util::SettingsValue>>& args) const;
};
extern ArgsManager gArgs;
bool HelpRequested(const ArgsManager& args);
void SetupHelpOptions(ArgsManager& args);
std::string HelpMessageGroup(const std::string& message);
std::string HelpMessageOpt(const std::string& option, const std::string& message);
int GetNumCores();
void ScheduleBatchPriority();
namespace util {
template <typename Tdst, typename Tsrc>
inline void insert(Tdst& dst, const Tsrc& src) {
    dst.insert(dst.begin(), src.begin(), src.end());
}
template <typename TsetT, typename Tsrc>
inline void insert(std::set<TsetT>& dst, const Tsrc& src) {
    dst.insert(src.begin(), src.end());
}
template<typename T>
T* AnyPtr(const std::any& any) noexcept
{
    T* const* ptr = std::any_cast<T*>(&any);
    return ptr ? *ptr : nullptr;
}
#ifdef WIN32
class WinCmdLineArgs
{
public:
    WinCmdLineArgs();
    ~WinCmdLineArgs();
    std::pair<int, char**> get();
private:
    int argc;
    char** argv;
    std::vector<std::string> args;
};
#endif
}
#endif
