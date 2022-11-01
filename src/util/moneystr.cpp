#include <util/moneystr.h>
#include <consensus/amount.h>
#include <tinyformat.h>
#include <util/strencodings.h>
#include <util/string.h>
#include <cstdint>
#include <optional>
std::string FormatMoney(const CAmount n)
{
    static_assert(COIN > 1);
    int64_t quotient = n / COIN;
    int64_t remainder = n % COIN;
    if (n < 0) {
        quotient = -quotient;
        remainder = -remainder;
    }
    std::string str = strprintf("%d.%08d", quotient, remainder);
    int nTrim = 0;
    for (int i = str.size()-1; (str[i] == '0' && IsDigit(str[i-2])); --i)
        ++nTrim;
    if (nTrim)
        str.erase(str.size()-nTrim, nTrim);
    if (n < 0)
        str.insert((unsigned int)0, 1, '-');
    return str;
}
std::optional<CAmount> ParseMoney(const std::string& money_string)
{
    if (!ContainsNoNUL(money_string)) {
        return std::nullopt;
    }
    const std::string str = TrimString(money_string);
    if (str.empty()) {
        return std::nullopt;
    }
    std::string strWhole;
    int64_t nUnits = 0;
    const char* p = str.c_str();
    for (; *p; p++)
    {
        if (*p == '.')
        {
            p++;
            int64_t nMult = COIN / 10;
            while (IsDigit(*p) && (nMult > 0))
            {
                nUnits += nMult * (*p++ - '0');
                nMult /= 10;
            }
            break;
        }
        if (IsSpace(*p))
            return std::nullopt;
        if (!IsDigit(*p))
            return std::nullopt;
        strWhole.insert(strWhole.end(), *p);
    }
    if (*p) {
        return std::nullopt;
    }
    if (strWhole.size() > 10)
        return std::nullopt;
    if (nUnits < 0 || nUnits > COIN)
        return std::nullopt;
    int64_t nWhole = LocaleIndependentAtoi<int64_t>(strWhole);
    CAmount value = nWhole * COIN + nUnits;
    if (!MoneyRange(value)) {
        return std::nullopt;
    }
    return value;
}
