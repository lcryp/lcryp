#include <support/cleanse.h>
#include <cstring>
#if defined(_MSC_VER)
#include <Windows.h>
#endif
void memory_cleanse(void *ptr, size_t len)
{
#if defined(_MSC_VER)
    SecureZeroMemory(ptr, len);
#else
    std::memset(ptr, 0, len);
    __asm__ __volatile__("" : : "r"(ptr) : "memory");
#endif
}
