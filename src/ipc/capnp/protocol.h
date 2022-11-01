#ifndef LCRYP_IPC_CAPNP_PROTOCOL_H
#define LCRYP_IPC_CAPNP_PROTOCOL_H
#include <memory>
namespace ipc {
class Protocol;
namespace capnp {
std::unique_ptr<Protocol> MakeCapnpProtocol();
}
}
#endif
