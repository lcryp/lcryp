#ifndef B2_SYSINFO_H
#define B2_SYSINFO_H
# include "config.h"
namespace b2
{
    class system_info
    {
        public:
        system_info();
        unsigned int cpu_core_count();
        unsigned int cpu_thread_count();
        private:
        unsigned int cpu_core_count_ = 0;
        unsigned int cpu_thread_count_ = 0;
    };
}
#endif
