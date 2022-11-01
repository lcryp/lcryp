#ifndef LCRYP_UTIL_OVERLOADED_H
#define LCRYP_UTIL_OVERLOADED_H
namespace util {
template<class... Ts> struct Overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> Overloaded(Ts...) -> Overloaded<Ts...>;
}
#endif
