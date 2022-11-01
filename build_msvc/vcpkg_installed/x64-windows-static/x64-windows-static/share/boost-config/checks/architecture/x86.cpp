#if !defined(i386) && !defined(__i386__) && !defined(__i386) \
    && !defined(__i486__) && !defined(__i586__) && !defined(__i686__) \
    && !defined(_M_IX86) && !defined(__X86__) && !defined(_X86_) \
    && !defined(__THW_INTEL__) && !defined(__I86__) && !defined(__INTEL__) \
    && !defined(__amd64__) && !defined(__x86_64__) && !defined(__amd64) \
    && !defined(__x86_64) && !defined(_M_X64)
#error "Not x86"
#endif
