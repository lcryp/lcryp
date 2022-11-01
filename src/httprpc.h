#ifndef LCRYP_HTTPRPC_H
#define LCRYP_HTTPRPC_H
#include <any>
bool StartHTTPRPC(const std::any& context);
void InterruptHTTPRPC();
void StopHTTPRPC();
void StartREST(const std::any& context);
void InterruptREST();
void StopREST();
#endif
