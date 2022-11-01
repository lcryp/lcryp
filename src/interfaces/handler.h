#ifndef LCRYP_INTERFACES_HANDLER_H
#define LCRYP_INTERFACES_HANDLER_H
#include <functional>
#include <memory>
namespace boost {
namespace signals2 {
class connection;
}
}
namespace interfaces {
class Handler
{
public:
    virtual ~Handler() {}
    virtual void disconnect() = 0;
};
std::unique_ptr<Handler> MakeHandler(boost::signals2::connection connection);
std::unique_ptr<Handler> MakeHandler(std::function<void()> cleanup);
}
#endif
