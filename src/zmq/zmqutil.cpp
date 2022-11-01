#include <zmq/zmqutil.h>
#include <logging.h>
#include <zmq.h>
#include <cerrno>
#include <string>
void zmqError(const std::string& str)
{
    LogPrint(BCLog::ZMQ, "Error: %s, msg: %s\n", str, zmq_strerror(errno));
}
