#include <fs.h>
#include <ipc/process.h>
#include <ipc/protocol.h>
#include <mp/util.h>
#include <tinyformat.h>
#include <util/strencodings.h>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string.h>
#include <system_error>
#include <unistd.h>
#include <utility>
#include <vector>
namespace ipc {
namespace {
class ProcessImpl : public Process
{
public:
    int spawn(const std::string& new_exe_name, const fs::path& argv0_path, int& pid) override
    {
        return mp::SpawnProcess(pid, [&](int fd) {
            fs::path path = argv0_path;
            path.remove_filename();
            path /= fs::PathFromString(new_exe_name);
            return std::vector<std::string>{fs::PathToString(path), "-ipcfd", strprintf("%i", fd)};
        });
    }
    int waitSpawned(int pid) override { return mp::WaitProcess(pid); }
    bool checkSpawned(int argc, char* argv[], int& fd) override
    {
        if (argc != 3 || strcmp(argv[1], "-ipcfd") != 0) {
            return false;
        }
        if (!ParseInt32(argv[2], &fd)) {
            throw std::runtime_error(strprintf("Invalid -ipcfd number '%s'", argv[2]));
        }
        return true;
    }
};
}
std::unique_ptr<Process> MakeProcess() { return std::make_unique<ProcessImpl>(); }
}
