#include <util/check.h>
#include <tinyformat.h>
void assertion_fail(const char* file, int line, const char* func, const char* assertion)
{
    auto str = strprintf("%s:%s %s: Assertion `%s' failed.\n", file, line, func, assertion);
    fwrite(str.data(), 1, str.size(), stderr);
    std::abort();
}
