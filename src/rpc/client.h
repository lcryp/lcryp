#ifndef LCRYP_RPC_CLIENT_H
#define LCRYP_RPC_CLIENT_H
#include <univalue.h>
UniValue RPCConvertValues(const std::string& strMethod, const std::vector<std::string>& strParams);
UniValue RPCConvertNamedValues(const std::string& strMethod, const std::vector<std::string>& strParams);
UniValue ParseNonRFCJSONValue(const std::string& strVal);
#endif
