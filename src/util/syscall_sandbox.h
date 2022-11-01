#ifndef LCRYP_UTIL_SYSCALL_SANDBOX_H
#define LCRYP_UTIL_SYSCALL_SANDBOX_H
enum class SyscallSandboxPolicy {
    INITIALIZATION,
    INITIALIZATION_DNS_SEED,
    INITIALIZATION_LOAD_BLOCKS,
    INITIALIZATION_MAP_PORT,
    MESSAGE_HANDLER,
    NET,
    NET_ADD_CONNECTION,
    NET_HTTP_SERVER,
    NET_HTTP_SERVER_WORKER,
    NET_OPEN_CONNECTION,
    SCHEDULER,
    TOR_CONTROL,
    TX_INDEX,
    VALIDATION_SCRIPT_CHECK,
    SHUTOFF,
};
void SetSyscallSandboxPolicy(SyscallSandboxPolicy syscall_policy);
#if defined(USE_SYSCALL_SANDBOX)
[[nodiscard]] bool SetupSyscallSandbox(bool log_syscall_violation_before_terminating);
void TestDisallowedSandboxCall();
#endif
#endif
