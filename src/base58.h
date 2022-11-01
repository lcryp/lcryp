#ifndef LCRYP_BASE58_H
#define LCRYP_BASE58_H
#include <span.h>
#include <string>
#include <vector>
std::string EncodeBase58(Span<const unsigned char> input);
[[nodiscard]] bool DecodeBase58(const std::string& str, std::vector<unsigned char>& vchRet, int max_ret_len);
std::string EncodeBase58Check(Span<const unsigned char> input);
[[nodiscard]] bool DecodeBase58Check(const std::string& str, std::vector<unsigned char>& vchRet, int max_ret_len);
#endif
