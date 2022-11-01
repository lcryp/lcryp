#ifndef LCRYP_IPC_EXCEPTION_H
#define LCRYP_IPC_EXCEPTION_H
#include <stdexcept>
namespace ipc {
class Exception : public std::runtime_error
{
public:
    using std::runtime_error::runtime_error;
};
}
#endif
