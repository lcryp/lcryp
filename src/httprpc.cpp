#include <httprpc.h>
#include <crypto/hmac_sha256.h>
#include <httpserver.h>
#include <rpc/protocol.h>
#include <rpc/server.h>
#include <util/strencodings.h>
#include <util/string.h>
#include <util/system.h>
#include <walletinitinterface.h>
#include <algorithm>
#include <iterator>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>
static const char* WWW_AUTH_HEADER_DATA = "Basic realm=\"jsonrpc\"";
class HTTPRPCTimer : public RPCTimerBase
{
public:
    HTTPRPCTimer(struct event_base* eventBase, std::function<void()>& func, int64_t millis) :
        ev(eventBase, false, func)
    {
        struct timeval tv;
        tv.tv_sec = millis/1000;
        tv.tv_usec = (millis%1000)*1000;
        ev.trigger(&tv);
    }
private:
    HTTPEvent ev;
};
class HTTPRPCTimerInterface : public RPCTimerInterface
{
public:
    explicit HTTPRPCTimerInterface(struct event_base* _base) : base(_base)
    {
    }
    const char* Name() override
    {
        return "HTTP";
    }
    RPCTimerBase* NewTimer(std::function<void()>& func, int64_t millis) override
    {
        return new HTTPRPCTimer(base, func, millis);
    }
private:
    struct event_base* base;
};
static std::string strRPCUserColonPass;
static std::unique_ptr<HTTPRPCTimerInterface> httpRPCTimerInterface;
static std::vector<std::vector<std::string>> g_rpcauth;
static std::map<std::string, std::set<std::string>> g_rpc_whitelist;
static bool g_rpc_whitelist_default = false;
static void JSONErrorReply(HTTPRequest* req, const UniValue& objError, const UniValue& id)
{
    int nStatus = HTTP_INTERNAL_SERVER_ERROR;
    int code = find_value(objError, "code").getInt<int>();
    if (code == RPC_INVALID_REQUEST)
        nStatus = HTTP_BAD_REQUEST;
    else if (code == RPC_METHOD_NOT_FOUND)
        nStatus = HTTP_NOT_FOUND;
    std::string strReply = JSONRPCReply(NullUniValue, objError, id);
    req->WriteHeader("Content-Type", "application/json");
    req->WriteReply(nStatus, strReply);
}
static bool multiUserAuthorized(std::string strUserPass)
{
    if (strUserPass.find(':') == std::string::npos) {
        return false;
    }
    std::string strUser = strUserPass.substr(0, strUserPass.find(':'));
    std::string strPass = strUserPass.substr(strUserPass.find(':') + 1);
    for (const auto& vFields : g_rpcauth) {
        std::string strName = vFields[0];
        if (!TimingResistantEqual(strName, strUser)) {
            continue;
        }
        std::string strSalt = vFields[1];
        std::string strHash = vFields[2];
        static const unsigned int KEY_SIZE = 32;
        unsigned char out[KEY_SIZE];
        CHMAC_SHA256(reinterpret_cast<const unsigned char*>(strSalt.data()), strSalt.size()).Write(reinterpret_cast<const unsigned char*>(strPass.data()), strPass.size()).Finalize(out);
        std::vector<unsigned char> hexvec(out, out+KEY_SIZE);
        std::string strHashFromPass = HexStr(hexvec);
        if (TimingResistantEqual(strHashFromPass, strHash)) {
            return true;
        }
    }
    return false;
}
static bool RPCAuthorized(const std::string& strAuth, std::string& strAuthUsernameOut)
{
    if (strRPCUserColonPass.empty())
        return false;
    if (strAuth.substr(0, 6) != "Basic ")
        return false;
    std::string_view strUserPass64 = TrimStringView(std::string_view{strAuth}.substr(6));
    auto userpass_data = DecodeBase64(strUserPass64);
    std::string strUserPass;
    if (!userpass_data) return false;
    strUserPass.assign(userpass_data->begin(), userpass_data->end());
    if (strUserPass.find(':') != std::string::npos)
        strAuthUsernameOut = strUserPass.substr(0, strUserPass.find(':'));
    if (TimingResistantEqual(strUserPass, strRPCUserColonPass)) {
        return true;
    }
    return multiUserAuthorized(strUserPass);
}
static bool HTTPReq_JSONRPC(const std::any& context, HTTPRequest* req)
{
    if (req->GetRequestMethod() != HTTPRequest::POST) {
        req->WriteReply(HTTP_BAD_METHOD, "JSONRPC server handles only POST requests");
        return false;
    }
    std::pair<bool, std::string> authHeader = req->GetHeader("authorization");
    if (!authHeader.first) {
        req->WriteHeader("WWW-Authenticate", WWW_AUTH_HEADER_DATA);
        req->WriteReply(HTTP_UNAUTHORIZED);
        return false;
    }
    JSONRPCRequest jreq;
    jreq.context = context;
    jreq.peerAddr = req->GetPeer().ToString();
    if (!RPCAuthorized(authHeader.second, jreq.authUser)) {
        LogPrintf("ThreadRPCServer incorrect password attempt from %s\n", jreq.peerAddr);
        UninterruptibleSleep(std::chrono::milliseconds{250});
        req->WriteHeader("WWW-Authenticate", WWW_AUTH_HEADER_DATA);
        req->WriteReply(HTTP_UNAUTHORIZED);
        return false;
    }
    try {
        UniValue valRequest;
        if (!valRequest.read(req->ReadBody()))
            throw JSONRPCError(RPC_PARSE_ERROR, "Parse error");
        jreq.URI = req->GetURI();
        std::string strReply;
        bool user_has_whitelist = g_rpc_whitelist.count(jreq.authUser);
        if (!user_has_whitelist && g_rpc_whitelist_default) {
            LogPrintf("RPC User %s not allowed to call any methods\n", jreq.authUser);
            req->WriteReply(HTTP_FORBIDDEN);
            return false;
        } else if (valRequest.isObject()) {
            jreq.parse(valRequest);
            if (user_has_whitelist && !g_rpc_whitelist[jreq.authUser].count(jreq.strMethod)) {
                LogPrintf("RPC User %s not allowed to call method %s\n", jreq.authUser, jreq.strMethod);
                req->WriteReply(HTTP_FORBIDDEN);
                return false;
            }
            UniValue result = tableRPC.execute(jreq);
            strReply = JSONRPCReply(result, NullUniValue, jreq.id);
        } else if (valRequest.isArray()) {
            if (user_has_whitelist) {
                for (unsigned int reqIdx = 0; reqIdx < valRequest.size(); reqIdx++) {
                    if (!valRequest[reqIdx].isObject()) {
                        throw JSONRPCError(RPC_INVALID_REQUEST, "Invalid Request object");
                    } else {
                        const UniValue& request = valRequest[reqIdx].get_obj();
                        std::string strMethod = find_value(request, "method").get_str();
                        if (!g_rpc_whitelist[jreq.authUser].count(strMethod)) {
                            LogPrintf("RPC User %s not allowed to call method %s\n", jreq.authUser, strMethod);
                            req->WriteReply(HTTP_FORBIDDEN);
                            return false;
                        }
                    }
                }
            }
            strReply = JSONRPCExecBatch(jreq, valRequest.get_array());
        }
        else
            throw JSONRPCError(RPC_PARSE_ERROR, "Top-level object parse error");
        req->WriteHeader("Content-Type", "application/json");
        req->WriteReply(HTTP_OK, strReply);
    } catch (const UniValue& objError) {
        JSONErrorReply(req, objError, jreq.id);
        return false;
    } catch (const std::exception& e) {
        JSONErrorReply(req, JSONRPCError(RPC_PARSE_ERROR, e.what()), jreq.id);
        return false;
    }
    return true;
}
static bool InitRPCAuthentication()
{
    if (gArgs.GetArg("-rpcpassword", "") == "")
    {
        LogPrintf("Using random cookie authentication.\n");
        if (!GenerateAuthCookie(&strRPCUserColonPass)) {
            return false;
        }
    } else {
        LogPrintf("Config options rpcuser and rpcpassword will soon be deprecated. Locally-run instances may remove rpcuser to use cookie-based auth, or may be replaced with rpcauth. Please see share/rpcauth for rpcauth auth generation.\n");
        strRPCUserColonPass = gArgs.GetArg("-rpcuser", "") + ":" + gArgs.GetArg("-rpcpassword", "");
    }
    if (gArgs.GetArg("-rpcauth", "") != "") {
        LogPrintf("Using rpcauth authentication.\n");
        for (const std::string& rpcauth : gArgs.GetArgs("-rpcauth")) {
            std::vector<std::string> fields{SplitString(rpcauth, ':')};
            const std::vector<std::string> salt_hmac{SplitString(fields.back(), '$')};
            if (fields.size() == 2 && salt_hmac.size() == 2) {
                fields.pop_back();
                fields.insert(fields.end(), salt_hmac.begin(), salt_hmac.end());
                g_rpcauth.push_back(fields);
            } else {
                LogPrintf("Invalid -rpcauth argument.\n");
                return false;
            }
        }
    }
    g_rpc_whitelist_default = gArgs.GetBoolArg("-rpcwhitelistdefault", gArgs.IsArgSet("-rpcwhitelist"));
    for (const std::string& strRPCWhitelist : gArgs.GetArgs("-rpcwhitelist")) {
        auto pos = strRPCWhitelist.find(':');
        std::string strUser = strRPCWhitelist.substr(0, pos);
        bool intersect = g_rpc_whitelist.count(strUser);
        std::set<std::string>& whitelist = g_rpc_whitelist[strUser];
        if (pos != std::string::npos) {
            std::string strWhitelist = strRPCWhitelist.substr(pos + 1);
            std::vector<std::string> whitelist_split = SplitString(strWhitelist, ", ");
            std::set<std::string> new_whitelist{
                std::make_move_iterator(whitelist_split.begin()),
                std::make_move_iterator(whitelist_split.end())};
            if (intersect) {
                std::set<std::string> tmp_whitelist;
                std::set_intersection(new_whitelist.begin(), new_whitelist.end(),
                       whitelist.begin(), whitelist.end(), std::inserter(tmp_whitelist, tmp_whitelist.end()));
                new_whitelist = std::move(tmp_whitelist);
            }
            whitelist = std::move(new_whitelist);
        }
    }
    return true;
}
bool StartHTTPRPC(const std::any& context)
{
    LogPrint(BCLog::RPC, "Starting HTTP RPC server\n");
    if (!InitRPCAuthentication())
        return false;
    auto handle_rpc = [context](HTTPRequest* req, const std::string&) { return HTTPReq_JSONRPC(context, req); };
    RegisterHTTPHandler("/", true, handle_rpc);
    if (g_wallet_init_interface.HasWalletSupport()) {
        RegisterHTTPHandler("/wallet/", false, handle_rpc);
    }
    struct event_base* eventBase = EventBase();
    assert(eventBase);
    httpRPCTimerInterface = std::make_unique<HTTPRPCTimerInterface>(eventBase);
    RPCSetTimerInterface(httpRPCTimerInterface.get());
    return true;
}
void InterruptHTTPRPC()
{
    LogPrint(BCLog::RPC, "Interrupting HTTP RPC server\n");
}
void StopHTTPRPC()
{
    LogPrint(BCLog::RPC, "Stopping HTTP RPC server\n");
    UnregisterHTTPHandler("/", true);
    if (g_wallet_init_interface.HasWalletSupport()) {
        UnregisterHTTPHandler("/wallet/", false);
    }
    if (httpRPCTimerInterface) {
        RPCUnsetTimerInterface(httpRPCTimerInterface.get());
        httpRPCTimerInterface.reset();
    }
}
