#ifndef LCRYP_KERNEL_CHECKS_H
#define LCRYP_KERNEL_CHECKS_H
#include <optional>
struct bilingual_str;
namespace kernel {
struct Context;
std::optional<bilingual_str> SanityChecks(const Context&);
}
#endif
