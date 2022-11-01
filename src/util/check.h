#ifndef LCRYP_UTIL_CHECK_H
#define LCRYP_UTIL_CHECK_H
#if defined(HAVE_CONFIG_H)
#include <config/lcryp-config.h>
#endif
#include <tinyformat.h>
#include <stdexcept>
class NonFatalCheckError : public std::runtime_error
{
    using std::runtime_error::runtime_error;
};
#define format_internal_error(msg, file, line, func, report)                                    \
    strprintf("Internal bug detected: \"%s\"\n%s:%d (%s)\nPlease report this issue here: %s\n", \
              msg, file, line, func, report)
template <typename T>
T&& inline_check_non_fatal(T&& val, const char* file, int line, const char* func, const char* assertion)
{
    if (!(val)) {
        throw NonFatalCheckError(
            format_internal_error(assertion, file, line, func, PACKAGE_BUGREPORT));
    }
    return std::forward<T>(val);
}
#define CHECK_NONFATAL(condition) \
    inline_check_non_fatal(condition, __FILE__, __LINE__, __func__, #condition)
#if defined(NDEBUG)
#error "Cannot compile without assertions!"
#endif
void assertion_fail(const char* file, int line, const char* func, const char* assertion);
template <bool IS_ASSERT, typename T>
T&& inline_assertion_check(T&& val, [[maybe_unused]] const char* file, [[maybe_unused]] int line, [[maybe_unused]] const char* func, [[maybe_unused]] const char* assertion)
{
    if constexpr (IS_ASSERT
#ifdef ABORT_ON_FAILED_ASSUME
                  || true
#endif
    ) {
        if (!val) {
            assertion_fail(file, line, func, assertion);
        }
    }
    return std::forward<T>(val);
}
#define Assert(val) inline_assertion_check<true>(val, __FILE__, __LINE__, __func__, #val)
#define Assume(val) inline_assertion_check<false>(val, __FILE__, __LINE__, __func__, #val)
#define NONFATAL_UNREACHABLE()                                        \
    throw NonFatalCheckError(                                         \
        format_internal_error("Unreachable code reached (non-fatal)", \
                              __FILE__, __LINE__, __func__, PACKAGE_BUGREPORT))
#endif
