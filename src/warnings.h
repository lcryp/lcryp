#ifndef LCRYP_WARNINGS_H
#define LCRYP_WARNINGS_H
#include <string>
struct bilingual_str;
void SetMiscWarning(const bilingual_str& warning);
void SetfLargeWorkInvalidChainFound(bool flag);
bilingual_str GetWarnings(bool verbose);
#endif
