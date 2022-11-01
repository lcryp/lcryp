#include <interfaces/echo.h>
#include <memory>
namespace interfaces {
namespace {
class EchoImpl : public Echo
{
public:
    std::string echo(const std::string& echo) override { return echo; }
};
}
std::unique_ptr<Echo> MakeEcho() { return std::make_unique<EchoImpl>(); }
}
