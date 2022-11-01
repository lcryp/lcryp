#ifndef LCRYP_UTIL_SETTINGS_H
#define LCRYP_UTIL_SETTINGS_H
#include <fs.h>
#include <map>
#include <string>
#include <vector>
class UniValue;
namespace util {
using SettingsValue = UniValue;
struct Settings {
    std::map<std::string, SettingsValue> forced_settings;
    std::map<std::string, std::vector<SettingsValue>> command_line_options;
    std::map<std::string, SettingsValue> rw_settings;
    std::map<std::string, std::map<std::string, std::vector<SettingsValue>>> ro_config;
};
bool ReadSettings(const fs::path& path,
    std::map<std::string, SettingsValue>& values,
    std::vector<std::string>& errors);
bool WriteSettings(const fs::path& path,
    const std::map<std::string, SettingsValue>& values,
    std::vector<std::string>& errors);
SettingsValue GetSetting(const Settings& settings,
    const std::string& section,
    const std::string& name,
    bool ignore_default_section_config,
    bool ignore_nonpersistent,
    bool get_chain_name);
std::vector<SettingsValue> GetSettingsList(const Settings& settings,
    const std::string& section,
    const std::string& name,
    bool ignore_default_section_config);
bool OnlyHasDefaultSectionSetting(const Settings& settings, const std::string& section, const std::string& name);
struct SettingsSpan {
    explicit SettingsSpan() = default;
    explicit SettingsSpan(const SettingsValue& value) noexcept : SettingsSpan(&value, 1) {}
    explicit SettingsSpan(const SettingsValue* data, size_t size) noexcept : data(data), size(size) {}
    explicit SettingsSpan(const std::vector<SettingsValue>& vec) noexcept;
    const SettingsValue* begin() const;
    const SettingsValue* end() const;
    bool empty() const;
    bool last_negated() const;
    size_t negated() const;
    const SettingsValue* data = nullptr;
    size_t size = 0;
};
template <typename Map, typename Key>
auto FindKey(Map&& map, Key&& key) -> decltype(&map.at(key))
{
    auto it = map.find(key);
    return it == map.end() ? nullptr : &it->second;
}
}
#endif
