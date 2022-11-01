#ifndef LCRYP_FS_H
#define LCRYP_FS_H
#include <tinyformat.h>
#include <cstdio>
#include <filesystem>
#include <functional>
#include <iomanip>
#include <ios>
#include <ostream>
#include <string>
#include <utility>
namespace fs {
using namespace std::filesystem;
class path : public std::filesystem::path
{
public:
    using std::filesystem::path::path;
    path(std::filesystem::path path) : std::filesystem::path::path(std::move(path)) {}
    path& operator=(std::filesystem::path path) { std::filesystem::path::operator=(std::move(path)); return *this; }
    path& operator/=(std::filesystem::path path) { std::filesystem::path::operator/=(std::move(path)); return *this; }
    path(const char* c) : std::filesystem::path(c) {}
    path& operator=(const char* c) { std::filesystem::path::operator=(c); return *this; }
    path& operator/=(const char* c) { std::filesystem::path::operator/=(c); return *this; }
    path& append(const char* c) { std::filesystem::path::append(c); return *this; }
    path(std::string) = delete;
    path& operator=(std::string) = delete;
    path& operator/=(std::string) = delete;
    path& append(std::string) = delete;
    std::string string() const = delete;
    std::string u8string() const
    {
        const auto& utf8_str{std::filesystem::path::u8string()};
        return std::string{utf8_str.begin(), utf8_str.end()};
    }
    path& make_preferred() { std::filesystem::path::make_preferred(); return *this; }
    path filename() const { return std::filesystem::path::filename(); }
};
static inline path u8path(const std::string& utf8_str)
{
#if __cplusplus < 202002L
    return std::filesystem::u8path(utf8_str);
#else
    return std::filesystem::path(std::u8string{utf8_str.begin(), utf8_str.end()});
#endif
}
static inline path absolute(const path& p)
{
    return std::filesystem::absolute(p);
}
static inline bool exists(const path& p)
{
    return std::filesystem::exists(p);
}
static inline auto quoted(const std::string& s)
{
    return std::quoted(s, '"', '&');
}
static inline path operator/(path p1, path p2)
{
    p1 /= std::move(p2);
    return p1;
}
static inline path operator/(path p1, const char* p2)
{
    p1 /= p2;
    return p1;
}
static inline path operator+(path p1, const char* p2)
{
    p1 += p2;
    return p1;
}
static inline path operator+(path p1, path::value_type p2)
{
    p1 += p2;
    return p1;
}
template<typename T> static inline path operator/(path p1, T p2) = delete;
template<typename T> static inline path operator+(path p1, T p2) = delete;
static inline bool copy_file(const path& from, const path& to, copy_options options)
{
    return std::filesystem::copy_file(from, to, options);
}
static inline std::string PathToString(const path& path)
{
#ifdef WIN32
    return path.u8string();
#else
    static_assert(std::is_same<path::string_type, std::string>::value, "PathToString not implemented on this platform");
    return path.std::filesystem::path::string();
#endif
}
static inline path PathFromString(const std::string& string)
{
#ifdef WIN32
    return u8path(string);
#else
    return std::filesystem::path(string);
#endif
}
static inline bool create_directories(const std::filesystem::path& p)
{
    if (std::filesystem::is_symlink(p) && std::filesystem::is_directory(p)) {
        return false;
    }
    return std::filesystem::create_directories(p);
}
bool create_directories(const std::filesystem::path& p, std::error_code& ec) = delete;
}
namespace fsbridge {
    using FopenFn = std::function<FILE*(const fs::path&, const char*)>;
    FILE *fopen(const fs::path& p, const char *mode);
    fs::path AbsPathJoin(const fs::path& base, const fs::path& path);
    class FileLock
    {
    public:
        FileLock() = delete;
        FileLock(const FileLock&) = delete;
        FileLock(FileLock&&) = delete;
        explicit FileLock(const fs::path& file);
        ~FileLock();
        bool TryLock();
        std::string GetReason() { return reason; }
    private:
        std::string reason;
#ifndef WIN32
        int fd = -1;
#else
        void* hFile = (void*)-1;
#endif
    };
    std::string get_filesystem_error_message(const fs::filesystem_error& e);
};
namespace tinyformat {
template<> inline void formatValue(std::ostream&, const char*, const char*, int, const std::filesystem::path&) = delete;
template<> inline void formatValue(std::ostream&, const char*, const char*, int, const fs::path&) = delete;
}
#endif
