#include <fs.h>
#include <util/settings.h>
#include <tinyformat.h>
#include <univalue.h>
#include <fstream>
#include <map>
#include <string>
#include <vector>
namespace util {
namespace {
enum class Source {
   FORCED,
   COMMAND_LINE,
   RW_SETTINGS,
   CONFIG_FILE_NETWORK_SECTION,
   CONFIG_FILE_DEFAULT_SECTION
};
template <typename Fn>
static void MergeSettings(const Settings& settings, const std::string& section, const std::string& name, Fn&& fn)
{
    if (auto* value = FindKey(settings.forced_settings, name)) {
        fn(SettingsSpan(*value), Source::FORCED);
    }
    if (auto* values = FindKey(settings.command_line_options, name)) {
        fn(SettingsSpan(*values), Source::COMMAND_LINE);
    }
    if (const SettingsValue* value = FindKey(settings.rw_settings, name)) {
        fn(SettingsSpan(*value), Source::RW_SETTINGS);
    }
    if (!section.empty()) {
        if (auto* map = FindKey(settings.ro_config, section)) {
            if (auto* values = FindKey(*map, name)) {
                fn(SettingsSpan(*values), Source::CONFIG_FILE_NETWORK_SECTION);
            }
        }
    }
    if (auto* map = FindKey(settings.ro_config, "")) {
        if (auto* values = FindKey(*map, name)) {
            fn(SettingsSpan(*values), Source::CONFIG_FILE_DEFAULT_SECTION);
        }
    }
}
}
bool ReadSettings(const fs::path& path, std::map<std::string, SettingsValue>& values, std::vector<std::string>& errors)
{
    values.clear();
    errors.clear();
    if (!fs::exists(path)) return true;
    std::ifstream file;
    file.open(path);
    if (!file.is_open()) {
      errors.emplace_back(strprintf("%s. Please check permissions.", fs::PathToString(path)));
      return false;
    }
    SettingsValue in;
    if (!in.read(std::string{std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()})) {
        errors.emplace_back(strprintf("Unable to parse settings file %s", fs::PathToString(path)));
        return false;
    }
    if (file.fail()) {
        errors.emplace_back(strprintf("Failed reading settings file %s", fs::PathToString(path)));
        return false;
    }
    file.close();
    if (!in.isObject()) {
        errors.emplace_back(strprintf("Found non-object value %s in settings file %s", in.write(), fs::PathToString(path)));
        return false;
    }
    const std::vector<std::string>& in_keys = in.getKeys();
    const std::vector<SettingsValue>& in_values = in.getValues();
    for (size_t i = 0; i < in_keys.size(); ++i) {
        auto inserted = values.emplace(in_keys[i], in_values[i]);
        if (!inserted.second) {
            errors.emplace_back(strprintf("Found duplicate key %s in settings file %s", in_keys[i], fs::PathToString(path)));
        }
    }
    return errors.empty();
}
bool WriteSettings(const fs::path& path,
    const std::map<std::string, SettingsValue>& values,
    std::vector<std::string>& errors)
{
    SettingsValue out(SettingsValue::VOBJ);
    for (const auto& value : values) {
        out.__pushKV(value.first, value.second);
    }
    std::ofstream file;
    file.open(path);
    if (file.fail()) {
        errors.emplace_back(strprintf("Error: Unable to open settings file %s for writing", fs::PathToString(path)));
        return false;
    }
    file << out.write(  4,   1) << std::endl;
    file.close();
    return true;
}
SettingsValue GetSetting(const Settings& settings,
    const std::string& section,
    const std::string& name,
    bool ignore_default_section_config,
    bool ignore_nonpersistent,
    bool get_chain_name)
{
    SettingsValue result;
    bool done = false;
    MergeSettings(settings, section, name, [&](SettingsSpan span, Source source) {
        const bool never_ignore_negated_setting = span.last_negated();
        const bool reverse_precedence =
            (source == Source::CONFIG_FILE_NETWORK_SECTION || source == Source::CONFIG_FILE_DEFAULT_SECTION) &&
            !get_chain_name;
        const bool skip_negated_command_line = get_chain_name;
        if (done) return;
        if (ignore_default_section_config && source == Source::CONFIG_FILE_DEFAULT_SECTION &&
            !never_ignore_negated_setting) {
            return;
        }
        if (ignore_nonpersistent && (source == Source::COMMAND_LINE || source == Source::FORCED)) return;
        if (skip_negated_command_line && span.last_negated()) return;
        if (!span.empty()) {
            result = reverse_precedence ? span.begin()[0] : span.end()[-1];
            done = true;
        } else if (span.last_negated()) {
            result = false;
            done = true;
        }
    });
    return result;
}
std::vector<SettingsValue> GetSettingsList(const Settings& settings,
    const std::string& section,
    const std::string& name,
    bool ignore_default_section_config)
{
    std::vector<SettingsValue> result;
    bool done = false;
    bool prev_negated_empty = false;
    MergeSettings(settings, section, name, [&](SettingsSpan span, Source source) {
        const bool add_zombie_config_values =
            (source == Source::CONFIG_FILE_NETWORK_SECTION || source == Source::CONFIG_FILE_DEFAULT_SECTION) &&
            !prev_negated_empty;
        if (ignore_default_section_config && source == Source::CONFIG_FILE_DEFAULT_SECTION) return;
        if (!done || add_zombie_config_values) {
            for (const auto& value : span) {
                if (value.isArray()) {
                    result.insert(result.end(), value.getValues().begin(), value.getValues().end());
                } else {
                    result.push_back(value);
                }
            }
        }
        done |= span.negated() > 0 || source == Source::FORCED;
        prev_negated_empty |= span.last_negated() && result.empty();
    });
    return result;
}
bool OnlyHasDefaultSectionSetting(const Settings& settings, const std::string& section, const std::string& name)
{
    bool has_default_section_setting = false;
    bool has_other_setting = false;
    MergeSettings(settings, section, name, [&](SettingsSpan span, Source source) {
        if (span.empty()) return;
        else if (source == Source::CONFIG_FILE_DEFAULT_SECTION) has_default_section_setting = true;
        else has_other_setting = true;
    });
    return has_default_section_setting && !has_other_setting;
}
SettingsSpan::SettingsSpan(const std::vector<SettingsValue>& vec) noexcept : SettingsSpan(vec.data(), vec.size()) {}
const SettingsValue* SettingsSpan::begin() const { return data + negated(); }
const SettingsValue* SettingsSpan::end() const { return data + size; }
bool SettingsSpan::empty() const { return size == 0 || last_negated(); }
bool SettingsSpan::last_negated() const { return size > 0 && data[size - 1].isFalse(); }
size_t SettingsSpan::negated() const
{
    for (size_t i = size; i > 0; --i) {
        if (data[i - 1].isFalse()) return i;
    }
    return 0;
}
}
