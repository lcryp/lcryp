#ifndef LCRYP_UTIL_TRANSLATION_H
#define LCRYP_UTIL_TRANSLATION_H
#include <tinyformat.h>
#include <functional>
#include <string>
struct bilingual_str {
    std::string original;
    std::string translated;
    bilingual_str& operator+=(const bilingual_str& rhs)
    {
        original += rhs.original;
        translated += rhs.translated;
        return *this;
    }
    bool empty() const
    {
        return original.empty();
    }
    void clear()
    {
        original.clear();
        translated.clear();
    }
};
inline bilingual_str operator+(bilingual_str lhs, const bilingual_str& rhs)
{
    lhs += rhs;
    return lhs;
}
inline bilingual_str Untranslated(std::string original) { return {original, original}; }
namespace tinyformat {
template <typename... Args>
bilingual_str format(const bilingual_str& fmt, const Args&... args)
{
    return bilingual_str{format(fmt.original, args...), format(fmt.translated, args...)};
}
}
const extern std::function<std::string(const char*)> G_TRANSLATION_FUN;
inline bilingual_str _(const char* psz)
{
    return bilingual_str{psz, G_TRANSLATION_FUN ? (G_TRANSLATION_FUN)(psz) : psz};
}
#endif
