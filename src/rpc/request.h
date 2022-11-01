#ifndef LCRYP_RPC_REQUEST_H
#define LCRYP_RPC_REQUEST_H
#include <any>
#include <string>
#include <univalue.h>
UniValue JSONRPCRequestObj(const std::string& strMethod, const UniValue& params, const UniValue& id);
UniValue JSONRPCReplyObj(const UniValue& result, const UniValue& error, const UniValue& id);
std::string JSONRPCReply(const UniValue& result, const UniValue& error, const UniValue& id);
UniValue JSONRPCError(int code, const std::string& message);
bool GenerateAuthCookie(std::string *cookie_out);
bool GetAuthCookie(std::string *cookie_out);
void DeleteAuthCookie();
std::vector<UniValue> JSONRPCProcessBatchReply(const UniValue& in);
class JSONRPCRequest
{
public:
    UniValue id;
    std::string strMethod;
    UniValue params;
    enum Mode { EXECUTE, GET_HELP, GET_ARGS } mode = EXECUTE;
    std::string URI;
    std::string authUser;
    std::string peerAddr;
    std::any context;
    void parse(const UniValue& valRequest);
};
#endif
