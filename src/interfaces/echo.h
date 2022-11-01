#ifndef LCRYP_INTERFACES_ECHO_H
#define LCRYP_INTERFACES_ECHO_H
#include <memory>
#include <string>
namespace interfaces {
class Echo
{
public:
    virtual ~Echo() {}
    virtual std::string echo(const std::string& echo) = 0;
};
std::unique_ptr<Echo> MakeEcho();
}
#endif
