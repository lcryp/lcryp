#if defined(HAVE_CONFIG_H)
#include <config/lcryp-config.h>
#endif
#include <util/syscall_sandbox.h>
#if defined(USE_SYSCALL_SANDBOX)
#include <array>
#include <cassert>
#include <cstdint>
#include <exception>
#include <map>
#include <new>
#include <set>
#include <string>
#include <vector>
#include <logging.h>
#include <tinyformat.h>
#include <util/threadnames.h>
#include <linux/audit.h>
#include <linux/filter.h>
#include <linux/seccomp.h>
#include <linux/unistd.h>
#include <signal.h>
#include <sys/prctl.h>
#include <sys/types.h>
#include <unistd.h>
namespace {
bool g_syscall_sandbox_enabled{false};
bool g_syscall_sandbox_log_violation_before_terminating{false};
#if !defined(__x86_64__)
#error Syscall sandbox is an experimental feature currently available only under Linux x86-64.
#endif
#ifndef SECCOMP_RET_KILL_PROCESS
#define SECCOMP_RET_KILL_PROCESS 0x80000000U
#endif
#ifndef __NR_clone3
#define __NR_clone3 435
#endif
#ifndef __NR_statx
#define __NR_statx 332
#endif
#ifndef __NR_getrandom
#define __NR_getrandom 318
#endif
#ifndef __NR_membarrier
#define __NR_membarrier 324
#endif
#ifndef __NR_copy_file_range
#define __NR_copy_file_range 326
#endif
#ifndef  __NR_rseq
#define __NR_rseq 334
#endif
const std::map<uint32_t, std::string> LINUX_SYSCALLS{
    {__NR_accept, "accept"},
    {__NR_accept4, "accept4"},
    {__NR_access, "access"},
    {__NR_acct, "acct"},
    {__NR_add_key, "add_key"},
    {__NR_adjtimex, "adjtimex"},
    {__NR_afs_syscall, "afs_syscall"},
    {__NR_alarm, "alarm"},
    {__NR_arch_prctl, "arch_prctl"},
    {__NR_bind, "bind"},
    {__NR_bpf, "bpf"},
    {__NR_brk, "brk"},
    {__NR_capget, "capget"},
    {__NR_capset, "capset"},
    {__NR_chdir, "chdir"},
    {__NR_chmod, "chmod"},
    {__NR_chown, "chown"},
    {__NR_chroot, "chroot"},
    {__NR_clock_adjtime, "clock_adjtime"},
    {__NR_clock_getres, "clock_getres"},
    {__NR_clock_gettime, "clock_gettime"},
    {__NR_clock_nanosleep, "clock_nanosleep"},
    {__NR_clock_settime, "clock_settime"},
    {__NR_clone, "clone"},
    {__NR_clone3, "clone3"},
    {__NR_close, "close"},
    {__NR_connect, "connect"},
    {__NR_copy_file_range, "copy_file_range"},
    {__NR_creat, "creat"},
    {__NR_create_module, "create_module"},
    {__NR_delete_module, "delete_module"},
    {__NR_dup, "dup"},
    {__NR_dup2, "dup2"},
    {__NR_dup3, "dup3"},
    {__NR_epoll_create, "epoll_create"},
    {__NR_epoll_create1, "epoll_create1"},
    {__NR_epoll_ctl, "epoll_ctl"},
    {__NR_epoll_ctl_old, "epoll_ctl_old"},
    {__NR_epoll_pwait, "epoll_pwait"},
    {__NR_epoll_wait, "epoll_wait"},
    {__NR_epoll_wait_old, "epoll_wait_old"},
    {__NR_eventfd, "eventfd"},
    {__NR_eventfd2, "eventfd2"},
    {__NR_execve, "execve"},
    {__NR_execveat, "execveat"},
    {__NR_exit, "exit"},
    {__NR_exit_group, "exit_group"},
    {__NR_faccessat, "faccessat"},
    {__NR_fadvise64, "fadvise64"},
    {__NR_fallocate, "fallocate"},
    {__NR_fanotify_init, "fanotify_init"},
    {__NR_fanotify_mark, "fanotify_mark"},
    {__NR_fchdir, "fchdir"},
    {__NR_fchmod, "fchmod"},
    {__NR_fchmodat, "fchmodat"},
    {__NR_fchown, "fchown"},
    {__NR_fchownat, "fchownat"},
    {__NR_fcntl, "fcntl"},
    {__NR_fdatasync, "fdatasync"},
    {__NR_fgetxattr, "fgetxattr"},
    {__NR_finit_module, "finit_module"},
    {__NR_flistxattr, "flistxattr"},
    {__NR_flock, "flock"},
    {__NR_fork, "fork"},
    {__NR_fremovexattr, "fremovexattr"},
    {__NR_fsetxattr, "fsetxattr"},
    {__NR_fstat, "fstat"},
    {__NR_fstatfs, "fstatfs"},
    {__NR_fsync, "fsync"},
    {__NR_ftruncate, "ftruncate"},
    {__NR_futex, "futex"},
    {__NR_futimesat, "futimesat"},
    {__NR_get_kernel_syms, "get_kernel_syms"},
    {__NR_get_mempolicy, "get_mempolicy"},
    {__NR_get_robust_list, "get_robust_list"},
    {__NR_get_thread_area, "get_thread_area"},
    {__NR_getcpu, "getcpu"},
    {__NR_getcwd, "getcwd"},
    {__NR_getdents, "getdents"},
    {__NR_getdents64, "getdents64"},
    {__NR_getegid, "getegid"},
    {__NR_geteuid, "geteuid"},
    {__NR_getgid, "getgid"},
    {__NR_getgroups, "getgroups"},
    {__NR_getitimer, "getitimer"},
    {__NR_getpeername, "getpeername"},
    {__NR_getpgid, "getpgid"},
    {__NR_getpgrp, "getpgrp"},
    {__NR_getpid, "getpid"},
    {__NR_getpmsg, "getpmsg"},
    {__NR_getppid, "getppid"},
    {__NR_getpriority, "getpriority"},
    {__NR_getrandom, "getrandom"},
    {__NR_getresgid, "getresgid"},
    {__NR_getresuid, "getresuid"},
    {__NR_getrlimit, "getrlimit"},
    {__NR_getrusage, "getrusage"},
    {__NR_getsid, "getsid"},
    {__NR_getsockname, "getsockname"},
    {__NR_getsockopt, "getsockopt"},
    {__NR_gettid, "gettid"},
    {__NR_gettimeofday, "gettimeofday"},
    {__NR_getuid, "getuid"},
    {__NR_getxattr, "getxattr"},
    {__NR_init_module, "init_module"},
    {__NR_inotify_add_watch, "inotify_add_watch"},
    {__NR_inotify_init, "inotify_init"},
    {__NR_inotify_init1, "inotify_init1"},
    {__NR_inotify_rm_watch, "inotify_rm_watch"},
    {__NR_io_cancel, "io_cancel"},
    {__NR_io_destroy, "io_destroy"},
    {__NR_io_getevents, "io_getevents"},
    {__NR_io_setup, "io_setup"},
    {__NR_io_submit, "io_submit"},
    {__NR_ioctl, "ioctl"},
    {__NR_ioperm, "ioperm"},
    {__NR_iopl, "iopl"},
    {__NR_ioprio_get, "ioprio_get"},
    {__NR_ioprio_set, "ioprio_set"},
    {__NR_kcmp, "kcmp"},
    {__NR_kexec_file_load, "kexec_file_load"},
    {__NR_kexec_load, "kexec_load"},
    {__NR_keyctl, "keyctl"},
    {__NR_kill, "kill"},
    {__NR_lchown, "lchown"},
    {__NR_lgetxattr, "lgetxattr"},
    {__NR_link, "link"},
    {__NR_linkat, "linkat"},
    {__NR_listen, "listen"},
    {__NR_listxattr, "listxattr"},
    {__NR_llistxattr, "llistxattr"},
    {__NR_lookup_dcookie, "lookup_dcookie"},
    {__NR_lremovexattr, "lremovexattr"},
    {__NR_lseek, "lseek"},
    {__NR_lsetxattr, "lsetxattr"},
    {__NR_lstat, "lstat"},
    {__NR_madvise, "madvise"},
    {__NR_mbind, "mbind"},
    {__NR_membarrier, "membarrier"},
    {__NR_memfd_create, "memfd_create"},
    {__NR_migrate_pages, "migrate_pages"},
    {__NR_mincore, "mincore"},
    {__NR_mkdir, "mkdir"},
    {__NR_mkdirat, "mkdirat"},
    {__NR_mknod, "mknod"},
    {__NR_mknodat, "mknodat"},
    {__NR_mlock, "mlock"},
    {__NR_mlock2, "mlock2"},
    {__NR_mlockall, "mlockall"},
    {__NR_mmap, "mmap"},
    {__NR_modify_ldt, "modify_ldt"},
    {__NR_mount, "mount"},
    {__NR_move_pages, "move_pages"},
    {__NR_mprotect, "mprotect"},
    {__NR_mq_getsetattr, "mq_getsetattr"},
    {__NR_mq_notify, "mq_notify"},
    {__NR_mq_open, "mq_open"},
    {__NR_mq_timedreceive, "mq_timedreceive"},
    {__NR_mq_timedsend, "mq_timedsend"},
    {__NR_mq_unlink, "mq_unlink"},
    {__NR_mremap, "mremap"},
    {__NR_msgctl, "msgctl"},
    {__NR_msgget, "msgget"},
    {__NR_msgrcv, "msgrcv"},
    {__NR_msgsnd, "msgsnd"},
    {__NR_msync, "msync"},
    {__NR_munlock, "munlock"},
    {__NR_munlockall, "munlockall"},
    {__NR_munmap, "munmap"},
    {__NR_name_to_handle_at, "name_to_handle_at"},
    {__NR_nanosleep, "nanosleep"},
    {__NR_newfstatat, "newfstatat"},
    {__NR_nfsservctl, "nfsservctl"},
    {__NR_open, "open"},
    {__NR_open_by_handle_at, "open_by_handle_at"},
    {__NR_openat, "openat"},
    {__NR_pause, "pause"},
    {__NR_perf_event_open, "perf_event_open"},
    {__NR_personality, "personality"},
    {__NR_pipe, "pipe"},
    {__NR_pipe2, "pipe2"},
    {__NR_pivot_root, "pivot_root"},
#ifdef __NR_pkey_alloc
    {__NR_pkey_alloc, "pkey_alloc"},
#endif
#ifdef __NR_pkey_free
    {__NR_pkey_free, "pkey_free"},
#endif
#ifdef __NR_pkey_mprotect
    {__NR_pkey_mprotect, "pkey_mprotect"},
#endif
    {__NR_poll, "poll"},
    {__NR_ppoll, "ppoll"},
    {__NR_prctl, "prctl"},
    {__NR_pread64, "pread64"},
    {__NR_preadv, "preadv"},
#ifdef __NR_preadv2
    {__NR_preadv2, "preadv2"},
#endif
    {__NR_prlimit64, "prlimit64"},
    {__NR_process_vm_readv, "process_vm_readv"},
    {__NR_process_vm_writev, "process_vm_writev"},
    {__NR_pselect6, "pselect6"},
    {__NR_ptrace, "ptrace"},
    {__NR_putpmsg, "putpmsg"},
    {__NR_pwrite64, "pwrite64"},
    {__NR_pwritev, "pwritev"},
#ifdef __NR_pwritev2
    {__NR_pwritev2, "pwritev2"},
#endif
    {__NR__sysctl, "_sysctl"},
    {__NR_query_module, "query_module"},
    {__NR_quotactl, "quotactl"},
    {__NR_read, "read"},
    {__NR_readahead, "readahead"},
    {__NR_readlink, "readlink"},
    {__NR_readlinkat, "readlinkat"},
    {__NR_readv, "readv"},
    {__NR_reboot, "reboot"},
    {__NR_recvfrom, "recvfrom"},
    {__NR_recvmmsg, "recvmmsg"},
    {__NR_recvmsg, "recvmsg"},
    {__NR_remap_file_pages, "remap_file_pages"},
    {__NR_removexattr, "removexattr"},
    {__NR_rename, "rename"},
    {__NR_renameat, "renameat"},
    {__NR_renameat2, "renameat2"},
    {__NR_request_key, "request_key"},
    {__NR_restart_syscall, "restart_syscall"},
    {__NR_rmdir, "rmdir"},
    {__NR_rseq, "rseq"},
    {__NR_rt_sigaction, "rt_sigaction"},
    {__NR_rt_sigpending, "rt_sigpending"},
    {__NR_rt_sigprocmask, "rt_sigprocmask"},
    {__NR_rt_sigqueueinfo, "rt_sigqueueinfo"},
    {__NR_rt_sigreturn, "rt_sigreturn"},
    {__NR_rt_sigsuspend, "rt_sigsuspend"},
    {__NR_rt_sigtimedwait, "rt_sigtimedwait"},
    {__NR_rt_tgsigqueueinfo, "rt_tgsigqueueinfo"},
    {__NR_sched_get_priority_max, "sched_get_priority_max"},
    {__NR_sched_get_priority_min, "sched_get_priority_min"},
    {__NR_sched_getaffinity, "sched_getaffinity"},
    {__NR_sched_getattr, "sched_getattr"},
    {__NR_sched_getparam, "sched_getparam"},
    {__NR_sched_getscheduler, "sched_getscheduler"},
    {__NR_sched_rr_get_interval, "sched_rr_get_interval"},
    {__NR_sched_setaffinity, "sched_setaffinity"},
    {__NR_sched_setattr, "sched_setattr"},
    {__NR_sched_setparam, "sched_setparam"},
    {__NR_sched_setscheduler, "sched_setscheduler"},
    {__NR_sched_yield, "sched_yield"},
    {__NR_seccomp, "seccomp"},
    {__NR_security, "security"},
    {__NR_select, "select"},
    {__NR_semctl, "semctl"},
    {__NR_semget, "semget"},
    {__NR_semop, "semop"},
    {__NR_semtimedop, "semtimedop"},
    {__NR_sendfile, "sendfile"},
    {__NR_sendmmsg, "sendmmsg"},
    {__NR_sendmsg, "sendmsg"},
    {__NR_sendto, "sendto"},
    {__NR_set_mempolicy, "set_mempolicy"},
    {__NR_set_robust_list, "set_robust_list"},
    {__NR_set_thread_area, "set_thread_area"},
    {__NR_set_tid_address, "set_tid_address"},
    {__NR_setdomainname, "setdomainname"},
    {__NR_setfsgid, "setfsgid"},
    {__NR_setfsuid, "setfsuid"},
    {__NR_setgid, "setgid"},
    {__NR_setgroups, "setgroups"},
    {__NR_sethostname, "sethostname"},
    {__NR_setitimer, "setitimer"},
    {__NR_setns, "setns"},
    {__NR_setpgid, "setpgid"},
    {__NR_setpriority, "setpriority"},
    {__NR_setregid, "setregid"},
    {__NR_setresgid, "setresgid"},
    {__NR_setresuid, "setresuid"},
    {__NR_setreuid, "setreuid"},
    {__NR_setrlimit, "setrlimit"},
    {__NR_setsid, "setsid"},
    {__NR_setsockopt, "setsockopt"},
    {__NR_settimeofday, "settimeofday"},
    {__NR_setuid, "setuid"},
    {__NR_setxattr, "setxattr"},
    {__NR_shmat, "shmat"},
    {__NR_shmctl, "shmctl"},
    {__NR_shmdt, "shmdt"},
    {__NR_shmget, "shmget"},
    {__NR_shutdown, "shutdown"},
    {__NR_sigaltstack, "sigaltstack"},
    {__NR_signalfd, "signalfd"},
    {__NR_signalfd4, "signalfd4"},
    {__NR_socket, "socket"},
    {__NR_socketpair, "socketpair"},
    {__NR_splice, "splice"},
    {__NR_stat, "stat"},
    {__NR_statfs, "statfs"},
    {__NR_statx, "statx"},
    {__NR_swapoff, "swapoff"},
    {__NR_swapon, "swapon"},
    {__NR_symlink, "symlink"},
    {__NR_symlinkat, "symlinkat"},
    {__NR_sync, "sync"},
    {__NR_sync_file_range, "sync_file_range"},
    {__NR_syncfs, "syncfs"},
    {__NR_sysfs, "sysfs"},
    {__NR_sysinfo, "sysinfo"},
    {__NR_syslog, "syslog"},
    {__NR_tee, "tee"},
    {__NR_tgkill, "tgkill"},
    {__NR_time, "time"},
    {__NR_timer_create, "timer_create"},
    {__NR_timer_delete, "timer_delete"},
    {__NR_timer_getoverrun, "timer_getoverrun"},
    {__NR_timer_gettime, "timer_gettime"},
    {__NR_timer_settime, "timer_settime"},
    {__NR_timerfd_create, "timerfd_create"},
    {__NR_timerfd_gettime, "timerfd_gettime"},
    {__NR_timerfd_settime, "timerfd_settime"},
    {__NR_times, "times"},
    {__NR_tkill, "tkill"},
    {__NR_truncate, "truncate"},
    {__NR_tuxcall, "tuxcall"},
    {__NR_umask, "umask"},
    {__NR_umount2, "umount2"},
    {__NR_uname, "uname"},
    {__NR_unlink, "unlink"},
    {__NR_unlinkat, "unlinkat"},
    {__NR_unshare, "unshare"},
    {__NR_uselib, "uselib"},
    {__NR_userfaultfd, "userfaultfd"},
    {__NR_ustat, "ustat"},
    {__NR_utime, "utime"},
    {__NR_utimensat, "utimensat"},
    {__NR_utimes, "utimes"},
    {__NR_vfork, "vfork"},
    {__NR_vhangup, "vhangup"},
    {__NR_vmsplice, "vmsplice"},
    {__NR_vserver, "vserver"},
    {__NR_wait4, "wait4"},
    {__NR_waitid, "waitid"},
    {__NR_write, "write"},
    {__NR_writev, "writev"},
};
std::string GetLinuxSyscallName(uint32_t syscall_number)
{
    const auto element = LINUX_SYSCALLS.find(syscall_number);
    if (element != LINUX_SYSCALLS.end()) {
        return element->second;
    }
    return "*unknown*";
}
void SyscallSandboxDebugSignalHandler(int, siginfo_t* signal_info, void* void_signal_context)
{
    constexpr int32_t SYS_SECCOMP_SI_CODE{1};
    assert(signal_info->si_code == SYS_SECCOMP_SI_CODE);
    const ucontext_t* signal_context = static_cast<ucontext_t*>(void_signal_context);
    assert(signal_context != nullptr);
    std::set_new_handler(std::terminate);
    const uint32_t syscall_number = static_cast<uint32_t>(signal_context->uc_mcontext.gregs[REG_RAX]);
    const std::string syscall_name = GetLinuxSyscallName(syscall_number);
    const std::string thread_name = !util::ThreadGetInternalName().empty() ? util::ThreadGetInternalName() : "*unnamed*";
    const std::string error_message = strprintf("ERROR: The syscall \"%s\" (syscall number %d) is not allowed by the syscall sandbox in thread \"%s\". Please report.", syscall_name, syscall_number, thread_name);
    tfm::format(std::cerr, "%s\n", error_message);
    LogPrintf("%s\n", error_message);
    std::terminate();
}
bool SetupSyscallSandboxDebugHandler()
{
    struct sigaction action = {};
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGSYS);
    action.sa_sigaction = &SyscallSandboxDebugSignalHandler;
    action.sa_flags = SA_SIGINFO;
    if (sigaction(SIGSYS, &action, nullptr) < 0) {
        return false;
    }
    if (sigprocmask(SIG_UNBLOCK, &mask, nullptr)) {
        return false;
    }
    return true;
}
enum class SyscallSandboxAction {
    KILL_PROCESS,
    INVOKE_SIGNAL_HANDLER,
};
class SeccompPolicyBuilder
{
    std::set<uint32_t> allowed_syscalls;
public:
    SeccompPolicyBuilder()
    {
        AllowAddressSpaceAccess();
        AllowEpoll();
        AllowEventFd();
        AllowFutex();
        AllowGeneralIo();
        AllowGetRandom();
        AllowGetSimpleId();
        AllowGetTime();
        AllowGlobalProcessEnvironment();
        AllowGlobalSystemStatus();
        AllowKernelInternalApi();
        AllowNetworkSocketInformation();
        AllowOperationOnExistingFileDescriptor();
        AllowPipe();
        AllowPrctl();
        AllowProcessStartOrDeath();
        AllowScheduling();
        AllowSignalHandling();
        AllowSleep();
        AllowUmask();
    }
    void AllowAddressSpaceAccess()
    {
        allowed_syscalls.insert(__NR_brk);
        allowed_syscalls.insert(__NR_madvise);
        allowed_syscalls.insert(__NR_membarrier);
        allowed_syscalls.insert(__NR_mincore);
        allowed_syscalls.insert(__NR_mlock);
        allowed_syscalls.insert(__NR_mmap);
        allowed_syscalls.insert(__NR_mprotect);
        allowed_syscalls.insert(__NR_mremap);
        allowed_syscalls.insert(__NR_munlock);
        allowed_syscalls.insert(__NR_munmap);
    }
    void AllowEpoll()
    {
        allowed_syscalls.insert(__NR_epoll_create1);
        allowed_syscalls.insert(__NR_epoll_ctl);
        allowed_syscalls.insert(__NR_epoll_pwait);
        allowed_syscalls.insert(__NR_epoll_wait);
    }
    void AllowEventFd()
    {
        allowed_syscalls.insert(__NR_eventfd2);
    }
    void AllowFileSystem()
    {
        allowed_syscalls.insert(__NR_access);
        allowed_syscalls.insert(__NR_chdir);
        allowed_syscalls.insert(__NR_chmod);
        allowed_syscalls.insert(__NR_copy_file_range);
        allowed_syscalls.insert(__NR_fallocate);
        allowed_syscalls.insert(__NR_fchmod);
        allowed_syscalls.insert(__NR_fchown);
        allowed_syscalls.insert(__NR_fdatasync);
        allowed_syscalls.insert(__NR_flock);
        allowed_syscalls.insert(__NR_fstat);
        allowed_syscalls.insert(__NR_fstatfs);
        allowed_syscalls.insert(__NR_fsync);
        allowed_syscalls.insert(__NR_ftruncate);
        allowed_syscalls.insert(__NR_getcwd);
        allowed_syscalls.insert(__NR_getdents);
        allowed_syscalls.insert(__NR_getdents64);
        allowed_syscalls.insert(__NR_lstat);
        allowed_syscalls.insert(__NR_mkdir);
        allowed_syscalls.insert(__NR_newfstatat);
        allowed_syscalls.insert(__NR_open);
        allowed_syscalls.insert(__NR_openat);
        allowed_syscalls.insert(__NR_readlink);
        allowed_syscalls.insert(__NR_rename);
        allowed_syscalls.insert(__NR_rmdir);
        allowed_syscalls.insert(__NR_sendfile);
        allowed_syscalls.insert(__NR_stat);
        allowed_syscalls.insert(__NR_statfs);
        allowed_syscalls.insert(__NR_statx);
        allowed_syscalls.insert(__NR_unlink);
        allowed_syscalls.insert(__NR_unlinkat);
    }
    void AllowFutex()
    {
        allowed_syscalls.insert(__NR_futex);
        allowed_syscalls.insert(__NR_set_robust_list);
    }
    void AllowGeneralIo()
    {
        allowed_syscalls.insert(__NR_ioctl);
        allowed_syscalls.insert(__NR_lseek);
        allowed_syscalls.insert(__NR_poll);
        allowed_syscalls.insert(__NR_ppoll);
        allowed_syscalls.insert(__NR_pread64);
        allowed_syscalls.insert(__NR_pwrite64);
        allowed_syscalls.insert(__NR_read);
        allowed_syscalls.insert(__NR_readv);
        allowed_syscalls.insert(__NR_recvfrom);
        allowed_syscalls.insert(__NR_recvmsg);
        allowed_syscalls.insert(__NR_select);
        allowed_syscalls.insert(__NR_sendmmsg);
        allowed_syscalls.insert(__NR_sendmsg);
        allowed_syscalls.insert(__NR_sendto);
        allowed_syscalls.insert(__NR_write);
        allowed_syscalls.insert(__NR_writev);
    }
    void AllowGetRandom()
    {
        allowed_syscalls.insert(__NR_getrandom);
    }
    void AllowGetSimpleId()
    {
        allowed_syscalls.insert(__NR_getegid);
        allowed_syscalls.insert(__NR_geteuid);
        allowed_syscalls.insert(__NR_getgid);
        allowed_syscalls.insert(__NR_getpgid);
        allowed_syscalls.insert(__NR_getpid);
        allowed_syscalls.insert(__NR_getppid);
        allowed_syscalls.insert(__NR_getresgid);
        allowed_syscalls.insert(__NR_getresuid);
        allowed_syscalls.insert(__NR_getsid);
        allowed_syscalls.insert(__NR_gettid);
        allowed_syscalls.insert(__NR_getuid);
    }
    void AllowGetTime()
    {
        allowed_syscalls.insert(__NR_clock_getres);
        allowed_syscalls.insert(__NR_clock_gettime);
        allowed_syscalls.insert(__NR_gettimeofday);
    }
    void AllowGlobalProcessEnvironment()
    {
        allowed_syscalls.insert(__NR_getrlimit);
        allowed_syscalls.insert(__NR_getrusage);
        allowed_syscalls.insert(__NR_prlimit64);
    }
    void AllowGlobalSystemStatus()
    {
        allowed_syscalls.insert(__NR_sysinfo);
        allowed_syscalls.insert(__NR_uname);
    }
    void AllowKernelInternalApi()
    {
        allowed_syscalls.insert(__NR_restart_syscall);
    }
    void AllowNetwork()
    {
        allowed_syscalls.insert(__NR_accept);
        allowed_syscalls.insert(__NR_accept4);
        allowed_syscalls.insert(__NR_bind);
        allowed_syscalls.insert(__NR_connect);
        allowed_syscalls.insert(__NR_listen);
        allowed_syscalls.insert(__NR_setsockopt);
        allowed_syscalls.insert(__NR_socket);
        allowed_syscalls.insert(__NR_socketpair);
    }
    void AllowNetworkSocketInformation()
    {
        allowed_syscalls.insert(__NR_getpeername);
        allowed_syscalls.insert(__NR_getsockname);
        allowed_syscalls.insert(__NR_getsockopt);
    }
    void AllowOperationOnExistingFileDescriptor()
    {
        allowed_syscalls.insert(__NR_close);
        allowed_syscalls.insert(__NR_dup);
        allowed_syscalls.insert(__NR_dup2);
        allowed_syscalls.insert(__NR_fcntl);
        allowed_syscalls.insert(__NR_shutdown);
    }
    void AllowPipe()
    {
        allowed_syscalls.insert(__NR_pipe);
        allowed_syscalls.insert(__NR_pipe2);
    }
    void AllowPrctl()
    {
        allowed_syscalls.insert(__NR_arch_prctl);
        allowed_syscalls.insert(__NR_prctl);
    }
    void AllowProcessStartOrDeath()
    {
        allowed_syscalls.insert(__NR_clone);
        allowed_syscalls.insert(__NR_clone3);
        allowed_syscalls.insert(__NR_exit);
        allowed_syscalls.insert(__NR_exit_group);
        allowed_syscalls.insert(__NR_fork);
        allowed_syscalls.insert(__NR_tgkill);
        allowed_syscalls.insert(__NR_wait4);
        allowed_syscalls.insert(__NR_rseq);
    }
    void AllowScheduling()
    {
        allowed_syscalls.insert(__NR_sched_getaffinity);
        allowed_syscalls.insert(__NR_sched_getparam);
        allowed_syscalls.insert(__NR_sched_getscheduler);
        allowed_syscalls.insert(__NR_sched_setscheduler);
        allowed_syscalls.insert(__NR_sched_yield);
    }
    void AllowSignalHandling()
    {
        allowed_syscalls.insert(__NR_rt_sigaction);
        allowed_syscalls.insert(__NR_rt_sigprocmask);
        allowed_syscalls.insert(__NR_rt_sigreturn);
        allowed_syscalls.insert(__NR_sigaltstack);
    }
    void AllowSleep()
    {
        allowed_syscalls.insert(__NR_clock_nanosleep);
        allowed_syscalls.insert(__NR_nanosleep);
    }
    void AllowUmask()
    {
        allowed_syscalls.insert(__NR_umask);
    }
    std::vector<sock_filter> BuildFilter(SyscallSandboxAction default_action)
    {
        std::vector<sock_filter> bpf_policy;
        bpf_policy.push_back(BPF_STMT(BPF_LD + BPF_W + BPF_ABS, offsetof(struct seccomp_data, arch)));
        bpf_policy.push_back(BPF_JUMP(BPF_JMP + BPF_JEQ + BPF_K, AUDIT_ARCH_X86_64, 1, 0));
        bpf_policy.push_back(BPF_STMT(BPF_RET + BPF_K, SECCOMP_RET_KILL_PROCESS));
        bpf_policy.push_back(BPF_STMT(BPF_LD + BPF_W + BPF_ABS, offsetof(struct seccomp_data, nr)));
        for (const uint32_t allowed_syscall : allowed_syscalls) {
            bpf_policy.push_back(BPF_JUMP(BPF_JMP + BPF_JEQ + BPF_K, allowed_syscall, 0, 1));
            bpf_policy.push_back(BPF_STMT(BPF_RET + BPF_K, SECCOMP_RET_ALLOW));
        }
        switch (default_action) {
        case SyscallSandboxAction::KILL_PROCESS:
            bpf_policy.push_back(BPF_STMT(BPF_RET + BPF_K, SECCOMP_RET_KILL_PROCESS));
            break;
        case SyscallSandboxAction::INVOKE_SIGNAL_HANDLER:
            bpf_policy.push_back(BPF_STMT(BPF_RET + BPF_K, SECCOMP_RET_TRAP));
            break;
        }
        return bpf_policy;
    }
};
}
bool SetupSyscallSandbox(bool log_syscall_violation_before_terminating)
{
    assert(!g_syscall_sandbox_enabled && "SetupSyscallSandbox(...) should only be called once.");
    g_syscall_sandbox_enabled = true;
    g_syscall_sandbox_log_violation_before_terminating = log_syscall_violation_before_terminating;
    if (log_syscall_violation_before_terminating) {
        if (!SetupSyscallSandboxDebugHandler()) {
            return false;
        }
    }
    return true;
}
void TestDisallowedSandboxCall()
{
    std::array<gid_t, 1> groups;
    [[maybe_unused]] int32_t ignored = getgroups(groups.size(), groups.data());
}
#endif
void SetSyscallSandboxPolicy(SyscallSandboxPolicy syscall_policy)
{
#if defined(USE_SYSCALL_SANDBOX)
    if (!g_syscall_sandbox_enabled) {
        return;
    }
    SeccompPolicyBuilder seccomp_policy_builder;
    switch (syscall_policy) {
    case SyscallSandboxPolicy::INITIALIZATION:
        seccomp_policy_builder.AllowFileSystem();
        seccomp_policy_builder.AllowNetwork();
        break;
    case SyscallSandboxPolicy::INITIALIZATION_DNS_SEED:
        seccomp_policy_builder.AllowFileSystem();
        seccomp_policy_builder.AllowNetwork();
        break;
    case SyscallSandboxPolicy::INITIALIZATION_LOAD_BLOCKS:
        seccomp_policy_builder.AllowFileSystem();
        break;
    case SyscallSandboxPolicy::INITIALIZATION_MAP_PORT:
        seccomp_policy_builder.AllowFileSystem();
        seccomp_policy_builder.AllowNetwork();
        break;
    case SyscallSandboxPolicy::MESSAGE_HANDLER:
        seccomp_policy_builder.AllowFileSystem();
        break;
    case SyscallSandboxPolicy::NET:
        seccomp_policy_builder.AllowFileSystem();
        seccomp_policy_builder.AllowNetwork();
        break;
    case SyscallSandboxPolicy::NET_ADD_CONNECTION:
        seccomp_policy_builder.AllowFileSystem();
        seccomp_policy_builder.AllowNetwork();
        break;
    case SyscallSandboxPolicy::NET_HTTP_SERVER:
        seccomp_policy_builder.AllowFileSystem();
        seccomp_policy_builder.AllowNetwork();
        break;
    case SyscallSandboxPolicy::NET_HTTP_SERVER_WORKER:
        seccomp_policy_builder.AllowFileSystem();
        seccomp_policy_builder.AllowNetwork();
        break;
    case SyscallSandboxPolicy::NET_OPEN_CONNECTION:
        seccomp_policy_builder.AllowFileSystem();
        seccomp_policy_builder.AllowNetwork();
        break;
    case SyscallSandboxPolicy::SCHEDULER:
        seccomp_policy_builder.AllowFileSystem();
        break;
    case SyscallSandboxPolicy::TOR_CONTROL:
        seccomp_policy_builder.AllowFileSystem();
        seccomp_policy_builder.AllowNetwork();
        break;
    case SyscallSandboxPolicy::TX_INDEX:
        seccomp_policy_builder.AllowFileSystem();
        break;
    case SyscallSandboxPolicy::VALIDATION_SCRIPT_CHECK:
        break;
    case SyscallSandboxPolicy::SHUTOFF:
        seccomp_policy_builder.AllowFileSystem();
        break;
    }
    const SyscallSandboxAction default_action = g_syscall_sandbox_log_violation_before_terminating ? SyscallSandboxAction::INVOKE_SIGNAL_HANDLER : SyscallSandboxAction::KILL_PROCESS;
    std::vector<sock_filter> filter = seccomp_policy_builder.BuildFilter(default_action);
    const sock_fprog prog = {
        .len = static_cast<uint16_t>(filter.size()),
        .filter = filter.data(),
    };
    if (prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0) != 0) {
        throw std::runtime_error("Syscall sandbox enforcement failed: prctl(PR_SET_NO_NEW_PRIVS)");
    }
    if (prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &prog) != 0) {
        throw std::runtime_error("Syscall sandbox enforcement failed: prctl(PR_SET_SECCOMP)");
    }
    const std::string thread_name = !util::ThreadGetInternalName().empty() ? util::ThreadGetInternalName() : "*unnamed*";
    LogPrint(BCLog::UTIL, "Syscall filter installed for thread \"%s\"\n", thread_name);
#endif
}
