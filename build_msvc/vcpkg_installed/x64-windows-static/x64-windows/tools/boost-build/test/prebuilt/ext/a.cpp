#ifdef _WIN32
__declspec(dllexport)
#endif
#ifdef RELEASE
void release() {}
#else
void debug() {}
#endif
