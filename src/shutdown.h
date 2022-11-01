#ifndef LCRYP_SHUTDOWN_H
#define LCRYP_SHUTDOWN_H
#include <util/translation.h>
bool AbortNode(const std::string& strMessage, bilingual_str user_message = bilingual_str{});
bool InitShutdownState();
void StartShutdown();
void AbortShutdown();
bool ShutdownRequested();
void WaitForShutdown();
#endif
