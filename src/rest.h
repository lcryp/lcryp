#ifndef LCRYP_REST_H
#define LCRYP_REST_H
#include <string>
enum class RESTResponseFormat {
    UNDEF,
    BINARY,
    HEX,
    JSON,
};
RESTResponseFormat ParseDataFormat(std::string& param, const std::string& strReq);
#endif
