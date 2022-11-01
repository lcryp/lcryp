#if !defined(__powerpc) && !defined(__powerpc__) && !defined(__ppc) \
    && !defined(__ppc__) && !defined(_M_PPC) && !defined(_ARCH_PPC) \
    && !defined(__POWERPC__) && !defined(__PPCGECKO__) \
    && !defined(__PPCBROADWAY) && !defined(_XENON)
#error "Not PPC"
#endif
