#ifndef LCRYP_NOUI_H
#define LCRYP_NOUI_H
#include <string>
struct bilingual_str;
bool noui_ThreadSafeMessageBox(const bilingual_str& message, const std::string& caption, unsigned int style);
bool noui_ThreadSafeQuestion(const bilingual_str&  , const std::string& message, const std::string& caption, unsigned int style);
void noui_InitMessage(const std::string& message);
void noui_connect();
void noui_test_redirect();
void noui_reconnect();
#endif
