#ifndef LCRYP_IPC_PROCESS_H
#define LCRYP_IPC_PROCESS_H
#include <memory>
#include <string>
namespace ipc {
class Protocol;
class Process
{
public:
    virtual ~Process() = default;
    virtual int spawn(const std::string& new_exe_name, const fs::path& argv0_path, int& pid) = 0;
    virtual int waitSpawned(int pid) = 0;
    virtual bool checkSpawned(int argc, char* argv[], int& fd) = 0;
};
std::unique_ptr<Process> MakeProcess();
}
#endif
