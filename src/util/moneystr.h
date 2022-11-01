#ifndef LCRYP_UTIL_MONEYSTR_H
#define LCRYP_UTIL_MONEYSTR_H
#include <consensus/amount.h>
#include <optional>
#include <string>
std::string FormatMoney(const CAmount n);
std::optional<CAmount> ParseMoney(const std::string& str);
#endif
