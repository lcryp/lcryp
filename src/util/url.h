#ifndef LCRYP_UTIL_URL_H
#define LCRYP_UTIL_URL_H
#include <string>
using UrlDecodeFn = std::string(const std::string& url_encoded);
UrlDecodeFn urlDecode;
extern UrlDecodeFn* const URL_DECODE;
#endif
