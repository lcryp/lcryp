#if !defined(__arm__) && !defined(__thumb__) && \
    !defined(__TARGET_ARCH_ARM) && !defined(__TARGET_ARCH_THUMB) && \
    !defined(_ARM) && !defined(_M_ARM) && \
    !defined(__aarch64__)
#error "Not ARM"
#endif
