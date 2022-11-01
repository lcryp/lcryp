#ifndef B2_STARTUP_H
#define B2_STARTUP_H
#include "config.h"
#include "frames.h"
#include <cstdlib>
namespace b2 {
namespace startup {
    void load_builtins();
    LIST* builtin_boost_build(FRAME* frame, int flags);
    bool bootstrap(FRAME* frame);
}
enum class exit_result : int {
#ifdef EXIT_SUCCESS
    success = EXIT_SUCCESS,
#else
    success = 0,
#endif
#ifdef EXIT_FAILURE
    failure = EXIT_FAILURE
#else
    failure = 1
#endif
};
inline void clean_exit(int exit_code)
{
    throw(exit_result) exit_code;
}
inline void clean_exit(exit_result exit_code)
{
    throw exit_code;
}
}
#endif
