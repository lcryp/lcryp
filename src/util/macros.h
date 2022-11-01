#ifndef LCRYP_UTIL_MACROS_H
#define LCRYP_UTIL_MACROS_H
#define PASTE(x, y) x ## y
#define PASTE2(x, y) PASTE(x, y)
#define UNIQUE_NAME(name) PASTE2(name, __COUNTER__)
#define STRINGIZE(X) DO_STRINGIZE(X)
#define DO_STRINGIZE(X) #X
#endif
