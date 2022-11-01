#ifndef LCRYP_SCRIPT_MINISCRIPT_H
#define LCRYP_SCRIPT_MINISCRIPT_H
#include <algorithm>
#include <functional>
#include <numeric>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <assert.h>
#include <cstdlib>
#include <policy/policy.h>
#include <primitives/transaction.h>
#include <script/script.h>
#include <span.h>
#include <util/spanparsing.h>
#include <util/strencodings.h>
#include <util/string.h>
#include <util/vector.h>
namespace miniscript {
class Type {
    uint32_t m_flags;
    explicit constexpr Type(uint32_t flags) : m_flags(flags) {}
public:
    friend constexpr Type operator"" _mst(const char* c, size_t l);
    constexpr Type operator|(Type x) const { return Type(m_flags | x.m_flags); }
    constexpr Type operator&(Type x) const { return Type(m_flags & x.m_flags); }
    constexpr bool operator<<(Type x) const { return (x.m_flags & ~m_flags) == 0; }
    constexpr bool operator<(Type x) const { return m_flags < x.m_flags; }
    constexpr bool operator==(Type x) const { return m_flags == x.m_flags; }
    constexpr Type If(bool x) const { return Type(x ? m_flags : 0); }
};
inline constexpr Type operator"" _mst(const char* c, size_t l) {
    Type typ{0};
    for (const char *p = c; p < c + l; p++) {
        typ = typ | Type(
            *p == 'B' ? 1 << 0 :
            *p == 'V' ? 1 << 1 :
            *p == 'K' ? 1 << 2 :
            *p == 'W' ? 1 << 3 :
            *p == 'z' ? 1 << 4 :
            *p == 'o' ? 1 << 5 :
            *p == 'n' ? 1 << 6 :
            *p == 'd' ? 1 << 7 :
            *p == 'u' ? 1 << 8 :
            *p == 'e' ? 1 << 9 :
            *p == 'f' ? 1 << 10 :
            *p == 's' ? 1 << 11 :
            *p == 'm' ? 1 << 12 :
            *p == 'x' ? 1 << 13 :
            *p == 'g' ? 1 << 14 :
            *p == 'h' ? 1 << 15 :
            *p == 'i' ? 1 << 16 :
            *p == 'j' ? 1 << 17 :
            *p == 'k' ? 1 << 18 :
            (throw std::logic_error("Unknown character in _mst literal"), 0)
        );
    }
    return typ;
}
using Opcode = std::pair<opcodetype, std::vector<unsigned char>>;
template<typename Key> struct Node;
template<typename Key> using NodeRef = std::shared_ptr<const Node<Key>>;
template<typename Key, typename... Args>
NodeRef<Key> MakeNodeRef(Args&&... args) { return std::make_shared<const Node<Key>>(std::forward<Args>(args)...); }
enum class Fragment {
    JUST_0,
    JUST_1,
    PK_K,
    PK_H,
    OLDER,
    AFTER,
    SHA256,
    HASH256,
    RIPEMD160,
    HASH160,
    WRAP_A,
    WRAP_S,
    WRAP_C,
    WRAP_D,
    WRAP_V,
    WRAP_J,
    WRAP_N,
    AND_V,
    AND_B,
    OR_B,
    OR_C,
    OR_D,
    OR_I,
    ANDOR,
    THRESH,
    MULTI,
};
namespace internal {
Type ComputeType(Fragment fragment, Type x, Type y, Type z, const std::vector<Type>& sub_types, uint32_t k, size_t data_size, size_t n_subs, size_t n_keys);
size_t ComputeScriptLen(Fragment fragment, Type sub0typ, size_t subsize, uint32_t k, size_t n_subs, size_t n_keys);
Type SanitizeType(Type x);
template<typename I>
struct MaxInt {
    const bool valid;
    const I value;
    MaxInt() : valid(false), value(0) {}
    MaxInt(I val) : valid(true), value(val) {}
    friend MaxInt<I> operator+(const MaxInt<I>& a, const MaxInt<I>& b) {
        if (!a.valid || !b.valid) return {};
        return a.value + b.value;
    }
    friend MaxInt<I> operator|(const MaxInt<I>& a, const MaxInt<I>& b) {
        if (!a.valid) return b;
        if (!b.valid) return a;
        return std::max(a.value, b.value);
    }
};
struct Ops {
    uint32_t count;
    MaxInt<uint32_t> sat;
    MaxInt<uint32_t> dsat;
    Ops(uint32_t in_count, MaxInt<uint32_t> in_sat, MaxInt<uint32_t> in_dsat) : count(in_count), sat(in_sat), dsat(in_dsat) {};
};
struct StackSize {
    MaxInt<uint32_t> sat;
    MaxInt<uint32_t> dsat;
    StackSize(MaxInt<uint32_t> in_sat, MaxInt<uint32_t> in_dsat) : sat(in_sat), dsat(in_dsat) {};
};
struct NoDupCheck {};
}
template<typename Key>
struct Node {
    const Fragment fragment;
    const uint32_t k = 0;
    const std::vector<Key> keys;
    const std::vector<unsigned char> data;
    const std::vector<NodeRef<Key>> subs;
private:
    const internal::Ops ops;
    const internal::StackSize ss;
    const Type typ;
    const size_t scriptlen;
    mutable std::optional<bool> has_duplicate_keys;
    size_t CalcScriptLen() const {
        size_t subsize = 0;
        for (const auto& sub : subs) {
            subsize += sub->ScriptSize();
        }
        Type sub0type = subs.size() > 0 ? subs[0]->GetType() : ""_mst;
        return internal::ComputeScriptLen(fragment, sub0type, subsize, k, subs.size(), keys.size());
    }
    template<typename Result, typename State, typename DownFn, typename UpFn>
    std::optional<Result> TreeEvalMaybe(State root_state, DownFn downfn, UpFn upfn) const
    {
        struct StackElem
        {
            const Node& node;
            size_t expanded;
            State state;
            StackElem(const Node& node_, size_t exp_, State&& state_) :
                node(node_), expanded(exp_), state(std::move(state_)) {}
        };
        std::vector<StackElem> stack;
        std::vector<Result> results;
        stack.emplace_back(*this, 0, std::move(root_state));
        while (stack.size()) {
            const Node& node = stack.back().node;
            if (stack.back().expanded < node.subs.size()) {
                size_t child_index = stack.back().expanded++;
                State child_state = downfn(stack.back().state, node, child_index);
                stack.emplace_back(*node.subs[child_index], 0, std::move(child_state));
                continue;
            }
            assert(results.size() >= node.subs.size());
            std::optional<Result> result{upfn(std::move(stack.back().state), node,
                Span<Result>{results}.last(node.subs.size()))};
            if (!result) return {};
            results.erase(results.end() - node.subs.size(), results.end());
            results.push_back(std::move(*result));
            stack.pop_back();
        }
        assert(results.size() == 1);
        return std::move(results[0]);
    }
    template<typename Result, typename UpFn>
    std::optional<Result> TreeEvalMaybe(UpFn upfn) const
    {
        struct DummyState {};
        return TreeEvalMaybe<Result>(DummyState{},
            [](DummyState, const Node&, size_t) { return DummyState{}; },
            [&upfn](DummyState, const Node& node, Span<Result> subs) {
                return upfn(node, subs);
            }
        );
    }
    template<typename Result, typename State, typename DownFn, typename UpFn>
    Result TreeEval(State root_state, DownFn&& downfn, UpFn upfn) const
    {
        return std::move(*TreeEvalMaybe<Result>(std::move(root_state),
            std::forward<DownFn>(downfn),
            [&upfn](State&& state, const Node& node, Span<Result> subs) {
                Result res{upfn(std::move(state), node, subs)};
                return std::optional<Result>(std::move(res));
            }
        ));
    }
    template<typename Result, typename UpFn>
    Result TreeEval(UpFn upfn) const
    {
        struct DummyState {};
        return std::move(*TreeEvalMaybe<Result>(DummyState{},
            [](DummyState, const Node&, size_t) { return DummyState{}; },
            [&upfn](DummyState, const Node& node, Span<Result> subs) {
                Result res{upfn(node, subs)};
                return std::optional<Result>(std::move(res));
            }
        ));
    }
    friend int Compare(const Node<Key>& node1, const Node<Key>& node2)
    {
        std::vector<std::pair<const Node<Key>&, const Node<Key>&>> queue;
        queue.emplace_back(node1, node2);
        while (!queue.empty()) {
            const auto& [a, b] = queue.back();
            queue.pop_back();
            if (std::tie(a.fragment, a.k, a.keys, a.data) < std::tie(b.fragment, b.k, b.keys, b.data)) return -1;
            if (std::tie(b.fragment, b.k, b.keys, b.data) < std::tie(a.fragment, a.k, a.keys, a.data)) return 1;
            if (a.subs.size() < b.subs.size()) return -1;
            if (b.subs.size() < a.subs.size()) return 1;
            size_t n = a.subs.size();
            for (size_t i = 0; i < n; ++i) {
                queue.emplace_back(*a.subs[n - 1 - i], *b.subs[n - 1 - i]);
            }
        }
        return 0;
    }
    Type CalcType() const {
        using namespace internal;
        std::vector<Type> sub_types;
        if (fragment == Fragment::THRESH) {
            for (const auto& sub : subs) sub_types.push_back(sub->GetType());
        }
        Type x = subs.size() > 0 ? subs[0]->GetType() : ""_mst;
        Type y = subs.size() > 1 ? subs[1]->GetType() : ""_mst;
        Type z = subs.size() > 2 ? subs[2]->GetType() : ""_mst;
        return SanitizeType(ComputeType(fragment, x, y, z, sub_types, k, data.size(), subs.size(), keys.size()));
    }
public:
    template<typename Ctx>
    CScript ToScript(const Ctx& ctx) const
    {
        auto downfn = [](bool verify, const Node& node, size_t index) {
            if (node.fragment == Fragment::WRAP_V) return true;
            if (node.fragment == Fragment::WRAP_S ||
                (node.fragment == Fragment::AND_V && index == 1)) return verify;
            return false;
        };
        auto upfn = [&ctx](bool verify, const Node& node, Span<CScript> subs) -> CScript {
            switch (node.fragment) {
                case Fragment::PK_K: return BuildScript(ctx.ToPKBytes(node.keys[0]));
                case Fragment::PK_H: return BuildScript(OP_DUP, OP_HASH160, ctx.ToPKHBytes(node.keys[0]), OP_EQUALVERIFY);
                case Fragment::OLDER: return BuildScript(node.k, OP_CHECKSEQUENCEVERIFY);
                case Fragment::AFTER: return BuildScript(node.k, OP_CHECKLOCKTIMEVERIFY);
                case Fragment::SHA256: return BuildScript(OP_SIZE, 32, OP_EQUALVERIFY, OP_SHA256, node.data, verify ? OP_EQUALVERIFY : OP_EQUAL);
                case Fragment::RIPEMD160: return BuildScript(OP_SIZE, 32, OP_EQUALVERIFY, OP_RIPEMD160, node.data, verify ? OP_EQUALVERIFY : OP_EQUAL);
                case Fragment::HASH256: return BuildScript(OP_SIZE, 32, OP_EQUALVERIFY, OP_HASH256, node.data, verify ? OP_EQUALVERIFY : OP_EQUAL);
                case Fragment::HASH160: return BuildScript(OP_SIZE, 32, OP_EQUALVERIFY, OP_HASH160, node.data, verify ? OP_EQUALVERIFY : OP_EQUAL);
                case Fragment::WRAP_A: return BuildScript(OP_TOALTSTACK, subs[0], OP_FROMALTSTACK);
                case Fragment::WRAP_S: return BuildScript(OP_SWAP, subs[0]);
                case Fragment::WRAP_C: return BuildScript(std::move(subs[0]), verify ? OP_CHECKSIGVERIFY : OP_CHECKSIG);
                case Fragment::WRAP_D: return BuildScript(OP_DUP, OP_IF, subs[0], OP_ENDIF);
                case Fragment::WRAP_V: {
                    if (node.subs[0]->GetType() << "x"_mst) {
                        return BuildScript(std::move(subs[0]), OP_VERIFY);
                    } else {
                        return std::move(subs[0]);
                    }
                }
                case Fragment::WRAP_J: return BuildScript(OP_SIZE, OP_0NOTEQUAL, OP_IF, subs[0], OP_ENDIF);
                case Fragment::WRAP_N: return BuildScript(std::move(subs[0]), OP_0NOTEQUAL);
                case Fragment::JUST_1: return BuildScript(OP_1);
                case Fragment::JUST_0: return BuildScript(OP_0);
                case Fragment::AND_V: return BuildScript(std::move(subs[0]), subs[1]);
                case Fragment::AND_B: return BuildScript(std::move(subs[0]), subs[1], OP_BOOLAND);
                case Fragment::OR_B: return BuildScript(std::move(subs[0]), subs[1], OP_BOOLOR);
                case Fragment::OR_D: return BuildScript(std::move(subs[0]), OP_IFDUP, OP_NOTIF, subs[1], OP_ENDIF);
                case Fragment::OR_C: return BuildScript(std::move(subs[0]), OP_NOTIF, subs[1], OP_ENDIF);
                case Fragment::OR_I: return BuildScript(OP_IF, subs[0], OP_ELSE, subs[1], OP_ENDIF);
                case Fragment::ANDOR: return BuildScript(std::move(subs[0]), OP_NOTIF, subs[2], OP_ELSE, subs[1], OP_ENDIF);
                case Fragment::MULTI: {
                    CScript script = BuildScript(node.k);
                    for (const auto& key : node.keys) {
                        script = BuildScript(std::move(script), ctx.ToPKBytes(key));
                    }
                    return BuildScript(std::move(script), node.keys.size(), verify ? OP_CHECKMULTISIGVERIFY : OP_CHECKMULTISIG);
                }
                case Fragment::THRESH: {
                    CScript script = std::move(subs[0]);
                    for (size_t i = 1; i < subs.size(); ++i) {
                        script = BuildScript(std::move(script), subs[i], OP_ADD);
                    }
                    return BuildScript(std::move(script), node.k, verify ? OP_EQUALVERIFY : OP_EQUAL);
                }
            }
            assert(false);
        };
        return TreeEval<CScript>(false, downfn, upfn);
    }
    template<typename CTx>
    std::optional<std::string> ToString(const CTx& ctx) const {
        auto downfn = [](bool, const Node& node, size_t) {
            return (node.fragment == Fragment::WRAP_A || node.fragment == Fragment::WRAP_S ||
                    node.fragment == Fragment::WRAP_D || node.fragment == Fragment::WRAP_V ||
                    node.fragment == Fragment::WRAP_J || node.fragment == Fragment::WRAP_N ||
                    node.fragment == Fragment::WRAP_C ||
                    (node.fragment == Fragment::AND_V && node.subs[1]->fragment == Fragment::JUST_1) ||
                    (node.fragment == Fragment::OR_I && node.subs[0]->fragment == Fragment::JUST_0) ||
                    (node.fragment == Fragment::OR_I && node.subs[1]->fragment == Fragment::JUST_0));
        };
        auto upfn = [&ctx](bool wrapped, const Node& node, Span<std::string> subs) -> std::optional<std::string> {
            std::string ret = wrapped ? ":" : "";
            switch (node.fragment) {
                case Fragment::WRAP_A: return "a" + std::move(subs[0]);
                case Fragment::WRAP_S: return "s" + std::move(subs[0]);
                case Fragment::WRAP_C:
                    if (node.subs[0]->fragment == Fragment::PK_K) {
                        auto key_str = ctx.ToString(node.subs[0]->keys[0]);
                        if (!key_str) return {};
                        return std::move(ret) + "pk(" + std::move(*key_str) + ")";
                    }
                    if (node.subs[0]->fragment == Fragment::PK_H) {
                        auto key_str = ctx.ToString(node.subs[0]->keys[0]);
                        if (!key_str) return {};
                        return std::move(ret) + "pkh(" + std::move(*key_str) + ")";
                    }
                    return "c" + std::move(subs[0]);
                case Fragment::WRAP_D: return "d" + std::move(subs[0]);
                case Fragment::WRAP_V: return "v" + std::move(subs[0]);
                case Fragment::WRAP_J: return "j" + std::move(subs[0]);
                case Fragment::WRAP_N: return "n" + std::move(subs[0]);
                case Fragment::AND_V:
                    if (node.subs[1]->fragment == Fragment::JUST_1) return "t" + std::move(subs[0]);
                    break;
                case Fragment::OR_I:
                    if (node.subs[0]->fragment == Fragment::JUST_0) return "l" + std::move(subs[1]);
                    if (node.subs[1]->fragment == Fragment::JUST_0) return "u" + std::move(subs[0]);
                    break;
                default: break;
            }
            switch (node.fragment) {
                case Fragment::PK_K: {
                    auto key_str = ctx.ToString(node.keys[0]);
                    if (!key_str) return {};
                    return std::move(ret) + "pk_k(" + std::move(*key_str) + ")";
                }
                case Fragment::PK_H: {
                    auto key_str = ctx.ToString(node.keys[0]);
                    if (!key_str) return {};
                    return std::move(ret) + "pk_h(" + std::move(*key_str) + ")";
                }
                case Fragment::AFTER: return std::move(ret) + "after(" + ::ToString(node.k) + ")";
                case Fragment::OLDER: return std::move(ret) + "older(" + ::ToString(node.k) + ")";
                case Fragment::HASH256: return std::move(ret) + "hash256(" + HexStr(node.data) + ")";
                case Fragment::HASH160: return std::move(ret) + "hash160(" + HexStr(node.data) + ")";
                case Fragment::SHA256: return std::move(ret) + "sha256(" + HexStr(node.data) + ")";
                case Fragment::RIPEMD160: return std::move(ret) + "ripemd160(" + HexStr(node.data) + ")";
                case Fragment::JUST_1: return std::move(ret) + "1";
                case Fragment::JUST_0: return std::move(ret) + "0";
                case Fragment::AND_V: return std::move(ret) + "and_v(" + std::move(subs[0]) + "," + std::move(subs[1]) + ")";
                case Fragment::AND_B: return std::move(ret) + "and_b(" + std::move(subs[0]) + "," + std::move(subs[1]) + ")";
                case Fragment::OR_B: return std::move(ret) + "or_b(" + std::move(subs[0]) + "," + std::move(subs[1]) + ")";
                case Fragment::OR_D: return std::move(ret) + "or_d(" + std::move(subs[0]) + "," + std::move(subs[1]) + ")";
                case Fragment::OR_C: return std::move(ret) + "or_c(" + std::move(subs[0]) + "," + std::move(subs[1]) + ")";
                case Fragment::OR_I: return std::move(ret) + "or_i(" + std::move(subs[0]) + "," + std::move(subs[1]) + ")";
                case Fragment::ANDOR:
                    if (node.subs[2]->fragment == Fragment::JUST_0) return std::move(ret) + "and_n(" + std::move(subs[0]) + "," + std::move(subs[1]) + ")";
                    return std::move(ret) + "andor(" + std::move(subs[0]) + "," + std::move(subs[1]) + "," + std::move(subs[2]) + ")";
                case Fragment::MULTI: {
                    auto str = std::move(ret) + "multi(" + ::ToString(node.k);
                    for (const auto& key : node.keys) {
                        auto key_str = ctx.ToString(key);
                        if (!key_str) return {};
                        str += "," + std::move(*key_str);
                    }
                    return std::move(str) + ")";
                }
                case Fragment::THRESH: {
                    auto str = std::move(ret) + "thresh(" + ::ToString(node.k);
                    for (auto& sub : subs) {
                        str += "," + std::move(sub);
                    }
                    return std::move(str) + ")";
                }
                default: break;
            }
            assert(false);
        };
        return TreeEvalMaybe<std::string>(false, downfn, upfn);
    }
private:
    internal::Ops CalcOps() const {
        switch (fragment) {
            case Fragment::JUST_1: return {0, 0, {}};
            case Fragment::JUST_0: return {0, {}, 0};
            case Fragment::PK_K: return {0, 0, 0};
            case Fragment::PK_H: return {3, 0, 0};
            case Fragment::OLDER:
            case Fragment::AFTER: return {1, 0, {}};
            case Fragment::SHA256:
            case Fragment::RIPEMD160:
            case Fragment::HASH256:
            case Fragment::HASH160: return {4, 0, {}};
            case Fragment::AND_V: return {subs[0]->ops.count + subs[1]->ops.count, subs[0]->ops.sat + subs[1]->ops.sat, {}};
            case Fragment::AND_B: {
                const auto count{1 + subs[0]->ops.count + subs[1]->ops.count};
                const auto sat{subs[0]->ops.sat + subs[1]->ops.sat};
                const auto dsat{subs[0]->ops.dsat + subs[1]->ops.dsat};
                return {count, sat, dsat};
            }
            case Fragment::OR_B: {
                const auto count{1 + subs[0]->ops.count + subs[1]->ops.count};
                const auto sat{(subs[0]->ops.sat + subs[1]->ops.dsat) | (subs[1]->ops.sat + subs[0]->ops.dsat)};
                const auto dsat{subs[0]->ops.dsat + subs[1]->ops.dsat};
                return {count, sat, dsat};
            }
            case Fragment::OR_D: {
                const auto count{3 + subs[0]->ops.count + subs[1]->ops.count};
                const auto sat{subs[0]->ops.sat | (subs[1]->ops.sat + subs[0]->ops.dsat)};
                const auto dsat{subs[0]->ops.dsat + subs[1]->ops.dsat};
                return {count, sat, dsat};
            }
            case Fragment::OR_C: {
                const auto count{2 + subs[0]->ops.count + subs[1]->ops.count};
                const auto sat{subs[0]->ops.sat | (subs[1]->ops.sat + subs[0]->ops.dsat)};
                return {count, sat, {}};
            }
            case Fragment::OR_I: {
                const auto count{3 + subs[0]->ops.count + subs[1]->ops.count};
                const auto sat{subs[0]->ops.sat | subs[1]->ops.sat};
                const auto dsat{subs[0]->ops.dsat | subs[1]->ops.dsat};
                return {count, sat, dsat};
            }
            case Fragment::ANDOR: {
                const auto count{3 + subs[0]->ops.count + subs[1]->ops.count + subs[2]->ops.count};
                const auto sat{(subs[1]->ops.sat + subs[0]->ops.sat) | (subs[0]->ops.dsat + subs[2]->ops.sat)};
                const auto dsat{subs[0]->ops.dsat + subs[2]->ops.dsat};
                return {count, sat, dsat};
            }
            case Fragment::MULTI: return {1, (uint32_t)keys.size(), (uint32_t)keys.size()};
            case Fragment::WRAP_S:
            case Fragment::WRAP_C:
            case Fragment::WRAP_N: return {1 + subs[0]->ops.count, subs[0]->ops.sat, subs[0]->ops.dsat};
            case Fragment::WRAP_A: return {2 + subs[0]->ops.count, subs[0]->ops.sat, subs[0]->ops.dsat};
            case Fragment::WRAP_D: return {3 + subs[0]->ops.count, subs[0]->ops.sat, 0};
            case Fragment::WRAP_J: return {4 + subs[0]->ops.count, subs[0]->ops.sat, 0};
            case Fragment::WRAP_V: return {subs[0]->ops.count + (subs[0]->GetType() << "x"_mst), subs[0]->ops.sat, {}};
            case Fragment::THRESH: {
                uint32_t count = 0;
                auto sats = Vector(internal::MaxInt<uint32_t>(0));
                for (const auto& sub : subs) {
                    count += sub->ops.count + 1;
                    auto next_sats = Vector(sats[0] + sub->ops.dsat);
                    for (size_t j = 1; j < sats.size(); ++j) next_sats.push_back((sats[j] + sub->ops.dsat) | (sats[j - 1] + sub->ops.sat));
                    next_sats.push_back(sats[sats.size() - 1] + sub->ops.sat);
                    sats = std::move(next_sats);
                }
                assert(k <= sats.size());
                return {count, sats[k], sats[0]};
            }
        }
        assert(false);
    }
    internal::StackSize CalcStackSize() const {
        switch (fragment) {
            case Fragment::JUST_0: return {{}, 0};
            case Fragment::JUST_1:
            case Fragment::OLDER:
            case Fragment::AFTER: return {0, {}};
            case Fragment::PK_K: return {1, 1};
            case Fragment::PK_H: return {2, 2};
            case Fragment::SHA256:
            case Fragment::RIPEMD160:
            case Fragment::HASH256:
            case Fragment::HASH160: return {1, {}};
            case Fragment::ANDOR: {
                const auto sat{(subs[0]->ss.sat + subs[1]->ss.sat) | (subs[0]->ss.dsat + subs[2]->ss.sat)};
                const auto dsat{subs[0]->ss.dsat + subs[2]->ss.dsat};
                return {sat, dsat};
            }
            case Fragment::AND_V: return {subs[0]->ss.sat + subs[1]->ss.sat, {}};
            case Fragment::AND_B: return {subs[0]->ss.sat + subs[1]->ss.sat, subs[0]->ss.dsat + subs[1]->ss.dsat};
            case Fragment::OR_B: {
                const auto sat{(subs[0]->ss.dsat + subs[1]->ss.sat) | (subs[0]->ss.sat + subs[1]->ss.dsat)};
                const auto dsat{subs[0]->ss.dsat + subs[1]->ss.dsat};
                return {sat, dsat};
            }
            case Fragment::OR_C: return {subs[0]->ss.sat | (subs[0]->ss.dsat + subs[1]->ss.sat), {}};
            case Fragment::OR_D: return {subs[0]->ss.sat | (subs[0]->ss.dsat + subs[1]->ss.sat), subs[0]->ss.dsat + subs[1]->ss.dsat};
            case Fragment::OR_I: return {(subs[0]->ss.sat + 1) | (subs[1]->ss.sat + 1), (subs[0]->ss.dsat + 1) | (subs[1]->ss.dsat + 1)};
            case Fragment::MULTI: return {k + 1, k + 1};
            case Fragment::WRAP_A:
            case Fragment::WRAP_N:
            case Fragment::WRAP_S:
            case Fragment::WRAP_C: return subs[0]->ss;
            case Fragment::WRAP_D: return {1 + subs[0]->ss.sat, 1};
            case Fragment::WRAP_V: return {subs[0]->ss.sat, {}};
            case Fragment::WRAP_J: return {subs[0]->ss.sat, 1};
            case Fragment::THRESH: {
                auto sats = Vector(internal::MaxInt<uint32_t>(0));
                for (const auto& sub : subs) {
                    auto next_sats = Vector(sats[0] + sub->ss.dsat);
                    for (size_t j = 1; j < sats.size(); ++j) next_sats.push_back((sats[j] + sub->ss.dsat) | (sats[j - 1] + sub->ss.sat));
                    next_sats.push_back(sats[sats.size() - 1] + sub->ss.sat);
                    sats = std::move(next_sats);
                }
                assert(k <= sats.size());
                return {sats[k], sats[0]};
            }
        }
        assert(false);
    }
public:
    template<typename Ctx> void DuplicateKeyCheck(const Ctx& ctx) const
    {
        struct Comp {
            const Ctx* ctx_ptr;
            Comp(const Ctx& ctx) : ctx_ptr(&ctx) {}
            bool operator()(const Key& a, const Key& b) const { return ctx_ptr->KeyCompare(a, b); }
        };
        using keyset = std::set<Key, Comp>;
        using state = std::optional<keyset>;
        auto upfn = [&ctx](const Node& node, Span<state> subs) -> state {
            if (node.has_duplicate_keys.has_value() && *node.has_duplicate_keys) return {};
            for (auto& sub : subs) {
                if (!sub.has_value()) {
                    node.has_duplicate_keys = true;
                    return {};
                }
            }
            size_t keys_count = node.keys.size();
            keyset key_set{node.keys.begin(), node.keys.end(), Comp(ctx)};
            if (key_set.size() != keys_count) {
                node.has_duplicate_keys = true;
                return {};
            }
            for (auto& sub : subs) {
                keys_count += sub->size();
                if (key_set.size() < sub->size()) std::swap(key_set, *sub);
                key_set.merge(*sub);
                if (key_set.size() != keys_count) {
                    node.has_duplicate_keys = true;
                    return {};
                }
            }
            node.has_duplicate_keys = false;
            return key_set;
        };
        TreeEval<state>(upfn);
    }
    size_t ScriptSize() const { return scriptlen; }
    uint32_t GetOps() const { return ops.count + ops.sat.value; }
    bool CheckOpsLimit() const { return GetOps() <= MAX_OPS_PER_SCRIPT; }
    uint32_t GetStackSize() const { return ss.sat.value + 1; }
    bool CheckStackSize() const { return GetStackSize() - 1 <= MAX_STANDARD_P2WSH_STACK_ITEMS; }
    Type GetType() const { return typ; }
    const Node* FindInsaneSub() const {
        return TreeEval<const Node*>([](const Node& node, Span<const Node*> subs) -> const Node* {
            for (auto& sub: subs) if (sub) return sub;
            if (!node.IsSaneSubexpression()) return &node;
            return nullptr;
        });
    }
    bool IsValid() const { return !(GetType() == ""_mst) && ScriptSize() <= MAX_STANDARD_P2WSH_SCRIPT_SIZE; }
    bool IsValidTopLevel() const { return IsValid() && GetType() << "B"_mst; }
    bool IsNonMalleable() const { return GetType() << "m"_mst; }
    bool NeedsSignature() const { return GetType() << "s"_mst; }
    bool CheckTimeLocksMix() const { return GetType() << "k"_mst; }
    bool CheckDuplicateKey() const { return has_duplicate_keys && !*has_duplicate_keys; }
    bool ValidSatisfactions() const { return IsValid() && CheckOpsLimit() && CheckStackSize(); }
    bool IsSaneSubexpression() const { return ValidSatisfactions() && IsNonMalleable() && CheckTimeLocksMix() && CheckDuplicateKey(); }
    bool IsSane() const { return IsValidTopLevel() && IsSaneSubexpression() && NeedsSignature(); }
    bool operator==(const Node<Key>& arg) const { return Compare(*this, arg) == 0; }
    Node(internal::NoDupCheck, Fragment nt, std::vector<NodeRef<Key>> sub, std::vector<unsigned char> arg, uint32_t val = 0) : fragment(nt), k(val), data(std::move(arg)), subs(std::move(sub)), ops(CalcOps()), ss(CalcStackSize()), typ(CalcType()), scriptlen(CalcScriptLen()) {}
    Node(internal::NoDupCheck, Fragment nt, std::vector<unsigned char> arg, uint32_t val = 0) : fragment(nt), k(val), data(std::move(arg)), ops(CalcOps()), ss(CalcStackSize()), typ(CalcType()), scriptlen(CalcScriptLen()) {}
    Node(internal::NoDupCheck, Fragment nt, std::vector<NodeRef<Key>> sub, std::vector<Key> key, uint32_t val = 0) : fragment(nt), k(val), keys(std::move(key)), subs(std::move(sub)), ops(CalcOps()), ss(CalcStackSize()), typ(CalcType()), scriptlen(CalcScriptLen()) {}
    Node(internal::NoDupCheck, Fragment nt, std::vector<Key> key, uint32_t val = 0) : fragment(nt), k(val), keys(std::move(key)), ops(CalcOps()), ss(CalcStackSize()), typ(CalcType()), scriptlen(CalcScriptLen()) {}
    Node(internal::NoDupCheck, Fragment nt, std::vector<NodeRef<Key>> sub, uint32_t val = 0) : fragment(nt), k(val), subs(std::move(sub)), ops(CalcOps()), ss(CalcStackSize()), typ(CalcType()), scriptlen(CalcScriptLen()) {}
    Node(internal::NoDupCheck, Fragment nt, uint32_t val = 0) : fragment(nt), k(val), ops(CalcOps()), ss(CalcStackSize()), typ(CalcType()), scriptlen(CalcScriptLen()) {}
    template <typename Ctx> Node(const Ctx& ctx, Fragment nt, std::vector<NodeRef<Key>> sub, std::vector<unsigned char> arg, uint32_t val = 0) : Node(internal::NoDupCheck{}, nt, std::move(sub), std::move(arg), val) { DuplicateKeyCheck(ctx); }
    template <typename Ctx> Node(const Ctx& ctx, Fragment nt, std::vector<unsigned char> arg, uint32_t val = 0) : Node(internal::NoDupCheck{}, nt, std::move(arg), val) { DuplicateKeyCheck(ctx);}
    template <typename Ctx> Node(const Ctx& ctx, Fragment nt, std::vector<NodeRef<Key>> sub, std::vector<Key> key, uint32_t val = 0) : Node(internal::NoDupCheck{}, nt, std::move(sub), std::move(key), val) { DuplicateKeyCheck(ctx); }
    template <typename Ctx> Node(const Ctx& ctx, Fragment nt, std::vector<Key> key, uint32_t val = 0) : Node(internal::NoDupCheck{}, nt, std::move(key), val) { DuplicateKeyCheck(ctx); }
    template <typename Ctx> Node(const Ctx& ctx, Fragment nt, std::vector<NodeRef<Key>> sub, uint32_t val = 0) : Node(internal::NoDupCheck{}, nt, std::move(sub), val) { DuplicateKeyCheck(ctx); }
    template <typename Ctx> Node(const Ctx& ctx, Fragment nt, uint32_t val = 0) : Node(internal::NoDupCheck{}, nt, val) { DuplicateKeyCheck(ctx); }
};
namespace internal {
enum class ParseContext {
    WRAPPED_EXPR,
    EXPR,
    SWAP,
    ALT,
    CHECK,
    DUP_IF,
    VERIFY,
    NON_ZERO,
    ZERO_NOTEQUAL,
    WRAP_U,
    WRAP_T,
    AND_N,
    AND_V,
    AND_B,
    ANDOR,
    OR_B,
    OR_C,
    OR_D,
    OR_I,
    THRESH,
    COMMA,
    CLOSE_BRACKET,
};
int FindNextChar(Span<const char> in, const char m);
template<typename Key, typename Ctx>
std::optional<std::pair<Key, int>> ParseKeyEnd(Span<const char> in, const Ctx& ctx)
{
    int key_size = FindNextChar(in, ')');
    if (key_size < 1) return {};
    auto key = ctx.FromString(in.begin(), in.begin() + key_size);
    if (!key) return {};
    return {{std::move(*key), key_size}};
}
template<typename Ctx>
std::optional<std::pair<std::vector<unsigned char>, int>> ParseHexStrEnd(Span<const char> in, const size_t expected_size,
                                                                         const Ctx& ctx)
{
    int hash_size = FindNextChar(in, ')');
    if (hash_size < 1) return {};
    std::string val = std::string(in.begin(), in.begin() + hash_size);
    if (!IsHex(val)) return {};
    auto hash = ParseHex(val);
    if (hash.size() != expected_size) return {};
    return {{std::move(hash), hash_size}};
}
template<typename Key>
void BuildBack(Fragment nt, std::vector<NodeRef<Key>>& constructed, const bool reverse = false)
{
    NodeRef<Key> child = std::move(constructed.back());
    constructed.pop_back();
    if (reverse) {
        constructed.back() = MakeNodeRef<Key>(internal::NoDupCheck{}, nt, Vector(std::move(child), std::move(constructed.back())));
    } else {
        constructed.back() = MakeNodeRef<Key>(internal::NoDupCheck{}, nt, Vector(std::move(constructed.back()), std::move(child)));
    }
}
template<typename Key, typename Ctx>
inline NodeRef<Key> Parse(Span<const char> in, const Ctx& ctx)
{
    using namespace spanparsing;
    size_t script_size{1};
    std::vector<std::tuple<ParseContext, int64_t, int64_t>> to_parse;
    std::vector<NodeRef<Key>> constructed;
    to_parse.emplace_back(ParseContext::WRAPPED_EXPR, -1, -1);
    while (!to_parse.empty()) {
        if (script_size > MAX_STANDARD_P2WSH_SCRIPT_SIZE) return {};
        auto [cur_context, n, k] = to_parse.back();
        to_parse.pop_back();
        switch (cur_context) {
        case ParseContext::WRAPPED_EXPR: {
            std::optional<size_t> colon_index{};
            for (size_t i = 1; i < in.size(); ++i) {
                if (in[i] == ':') {
                    colon_index = i;
                    break;
                }
                if (in[i] < 'a' || in[i] > 'z') break;
            }
            bool last_was_v{false};
            for (size_t j = 0; colon_index && j < *colon_index; ++j) {
                if (script_size > MAX_STANDARD_P2WSH_SCRIPT_SIZE) return {};
                if (in[j] == 'a') {
                    script_size += 2;
                    to_parse.emplace_back(ParseContext::ALT, -1, -1);
                } else if (in[j] == 's') {
                    script_size += 1;
                    to_parse.emplace_back(ParseContext::SWAP, -1, -1);
                } else if (in[j] == 'c') {
                    script_size += 1;
                    to_parse.emplace_back(ParseContext::CHECK, -1, -1);
                } else if (in[j] == 'd') {
                    script_size += 3;
                    to_parse.emplace_back(ParseContext::DUP_IF, -1, -1);
                } else if (in[j] == 'j') {
                    script_size += 4;
                    to_parse.emplace_back(ParseContext::NON_ZERO, -1, -1);
                } else if (in[j] == 'n') {
                    script_size += 1;
                    to_parse.emplace_back(ParseContext::ZERO_NOTEQUAL, -1, -1);
                } else if (in[j] == 'v') {
                    if (last_was_v) return {};
                    to_parse.emplace_back(ParseContext::VERIFY, -1, -1);
                } else if (in[j] == 'u') {
                    script_size += 4;
                    to_parse.emplace_back(ParseContext::WRAP_U, -1, -1);
                } else if (in[j] == 't') {
                    script_size += 1;
                    to_parse.emplace_back(ParseContext::WRAP_T, -1, -1);
                } else if (in[j] == 'l') {
                    script_size += 4;
                    constructed.push_back(MakeNodeRef<Key>(internal::NoDupCheck{}, Fragment::JUST_0));
                    to_parse.emplace_back(ParseContext::OR_I, -1, -1);
                } else {
                    return {};
                }
                last_was_v = (in[j] == 'v');
            }
            to_parse.emplace_back(ParseContext::EXPR, -1, -1);
            if (colon_index) in = in.subspan(*colon_index + 1);
            break;
        }
        case ParseContext::EXPR: {
            if (Const("0", in)) {
                constructed.push_back(MakeNodeRef<Key>(internal::NoDupCheck{}, Fragment::JUST_0));
            } else if (Const("1", in)) {
                constructed.push_back(MakeNodeRef<Key>(internal::NoDupCheck{}, Fragment::JUST_1));
            } else if (Const("pk(", in)) {
                auto res = ParseKeyEnd<Key, Ctx>(in, ctx);
                if (!res) return {};
                auto& [key, key_size] = *res;
                constructed.push_back(MakeNodeRef<Key>(internal::NoDupCheck{}, Fragment::WRAP_C, Vector(MakeNodeRef<Key>(internal::NoDupCheck{}, Fragment::PK_K, Vector(std::move(key))))));
                in = in.subspan(key_size + 1);
                script_size += 34;
            } else if (Const("pkh(", in)) {
                auto res = ParseKeyEnd<Key>(in, ctx);
                if (!res) return {};
                auto& [key, key_size] = *res;
                constructed.push_back(MakeNodeRef<Key>(internal::NoDupCheck{}, Fragment::WRAP_C, Vector(MakeNodeRef<Key>(internal::NoDupCheck{}, Fragment::PK_H, Vector(std::move(key))))));
                in = in.subspan(key_size + 1);
                script_size += 24;
            } else if (Const("pk_k(", in)) {
                auto res = ParseKeyEnd<Key>(in, ctx);
                if (!res) return {};
                auto& [key, key_size] = *res;
                constructed.push_back(MakeNodeRef<Key>(internal::NoDupCheck{}, Fragment::PK_K, Vector(std::move(key))));
                in = in.subspan(key_size + 1);
                script_size += 33;
            } else if (Const("pk_h(", in)) {
                auto res = ParseKeyEnd<Key>(in, ctx);
                if (!res) return {};
                auto& [key, key_size] = *res;
                constructed.push_back(MakeNodeRef<Key>(internal::NoDupCheck{}, Fragment::PK_H, Vector(std::move(key))));
                in = in.subspan(key_size + 1);
                script_size += 23;
            } else if (Const("sha256(", in)) {
                auto res = ParseHexStrEnd(in, 32, ctx);
                if (!res) return {};
                auto& [hash, hash_size] = *res;
                constructed.push_back(MakeNodeRef<Key>(internal::NoDupCheck{}, Fragment::SHA256, std::move(hash)));
                in = in.subspan(hash_size + 1);
                script_size += 38;
            } else if (Const("ripemd160(", in)) {
                auto res = ParseHexStrEnd(in, 20, ctx);
                if (!res) return {};
                auto& [hash, hash_size] = *res;
                constructed.push_back(MakeNodeRef<Key>(internal::NoDupCheck{}, Fragment::RIPEMD160, std::move(hash)));
                in = in.subspan(hash_size + 1);
                script_size += 26;
            } else if (Const("hash256(", in)) {
                auto res = ParseHexStrEnd(in, 32, ctx);
                if (!res) return {};
                auto& [hash, hash_size] = *res;
                constructed.push_back(MakeNodeRef<Key>(internal::NoDupCheck{}, Fragment::HASH256, std::move(hash)));
                in = in.subspan(hash_size + 1);
                script_size += 38;
            } else if (Const("hash160(", in)) {
                auto res = ParseHexStrEnd(in, 20, ctx);
                if (!res) return {};
                auto& [hash, hash_size] = *res;
                constructed.push_back(MakeNodeRef<Key>(internal::NoDupCheck{}, Fragment::HASH160, std::move(hash)));
                in = in.subspan(hash_size + 1);
                script_size += 26;
            } else if (Const("after(", in)) {
                int arg_size = FindNextChar(in, ')');
                if (arg_size < 1) return {};
                int64_t num;
                if (!ParseInt64(std::string(in.begin(), in.begin() + arg_size), &num)) return {};
                if (num < 1 || num >= 0x80000000L) return {};
                constructed.push_back(MakeNodeRef<Key>(internal::NoDupCheck{}, Fragment::AFTER, num));
                in = in.subspan(arg_size + 1);
                script_size += 1 + (num > 16) + (num > 0x7f) + (num > 0x7fff) + (num > 0x7fffff);
            } else if (Const("older(", in)) {
                int arg_size = FindNextChar(in, ')');
                if (arg_size < 1) return {};
                int64_t num;
                if (!ParseInt64(std::string(in.begin(), in.begin() + arg_size), &num)) return {};
                if (num < 1 || num >= 0x80000000L) return {};
                constructed.push_back(MakeNodeRef<Key>(internal::NoDupCheck{}, Fragment::OLDER, num));
                in = in.subspan(arg_size + 1);
                script_size += 1 + (num > 16) + (num > 0x7f) + (num > 0x7fff) + (num > 0x7fffff);
            } else if (Const("multi(", in)) {
                int next_comma = FindNextChar(in, ',');
                if (next_comma < 1) return {};
                if (!ParseInt64(std::string(in.begin(), in.begin() + next_comma), &k)) return {};
                in = in.subspan(next_comma + 1);
                std::vector<Key> keys;
                while (next_comma != -1) {
                    next_comma = FindNextChar(in, ',');
                    int key_length = (next_comma == -1) ? FindNextChar(in, ')') : next_comma;
                    if (key_length < 1) return {};
                    auto key = ctx.FromString(in.begin(), in.begin() + key_length);
                    if (!key) return {};
                    keys.push_back(std::move(*key));
                    in = in.subspan(key_length + 1);
                }
                if (keys.size() < 1 || keys.size() > 20) return {};
                if (k < 1 || k > (int64_t)keys.size()) return {};
                script_size += 2 + (keys.size() > 16) + (k > 16) + 34 * keys.size();
                constructed.push_back(MakeNodeRef<Key>(internal::NoDupCheck{}, Fragment::MULTI, std::move(keys), k));
            } else if (Const("thresh(", in)) {
                int next_comma = FindNextChar(in, ',');
                if (next_comma < 1) return {};
                if (!ParseInt64(std::string(in.begin(), in.begin() + next_comma), &k)) return {};
                if (k < 1) return {};
                in = in.subspan(next_comma + 1);
                to_parse.emplace_back(ParseContext::THRESH, 1, k);
                to_parse.emplace_back(ParseContext::WRAPPED_EXPR, -1, -1);
                script_size += 2 + (k > 16) + (k > 0x7f) + (k > 0x7fff) + (k > 0x7fffff);
            } else if (Const("andor(", in)) {
                to_parse.emplace_back(ParseContext::ANDOR, -1, -1);
                to_parse.emplace_back(ParseContext::CLOSE_BRACKET, -1, -1);
                to_parse.emplace_back(ParseContext::WRAPPED_EXPR, -1, -1);
                to_parse.emplace_back(ParseContext::COMMA, -1, -1);
                to_parse.emplace_back(ParseContext::WRAPPED_EXPR, -1, -1);
                to_parse.emplace_back(ParseContext::COMMA, -1, -1);
                to_parse.emplace_back(ParseContext::WRAPPED_EXPR, -1, -1);
                script_size += 5;
            } else {
                if (Const("and_n(", in)) {
                    to_parse.emplace_back(ParseContext::AND_N, -1, -1);
                    script_size += 5;
                } else if (Const("and_b(", in)) {
                    to_parse.emplace_back(ParseContext::AND_B, -1, -1);
                    script_size += 2;
                } else if (Const("and_v(", in)) {
                    to_parse.emplace_back(ParseContext::AND_V, -1, -1);
                    script_size += 1;
                } else if (Const("or_b(", in)) {
                    to_parse.emplace_back(ParseContext::OR_B, -1, -1);
                    script_size += 2;
                } else if (Const("or_c(", in)) {
                    to_parse.emplace_back(ParseContext::OR_C, -1, -1);
                    script_size += 3;
                } else if (Const("or_d(", in)) {
                    to_parse.emplace_back(ParseContext::OR_D, -1, -1);
                    script_size += 4;
                } else if (Const("or_i(", in)) {
                    to_parse.emplace_back(ParseContext::OR_I, -1, -1);
                    script_size += 4;
                } else {
                    return {};
                }
                to_parse.emplace_back(ParseContext::CLOSE_BRACKET, -1, -1);
                to_parse.emplace_back(ParseContext::WRAPPED_EXPR, -1, -1);
                to_parse.emplace_back(ParseContext::COMMA, -1, -1);
                to_parse.emplace_back(ParseContext::WRAPPED_EXPR, -1, -1);
            }
            break;
        }
        case ParseContext::ALT: {
            constructed.back() = MakeNodeRef<Key>(internal::NoDupCheck{}, Fragment::WRAP_A, Vector(std::move(constructed.back())));
            break;
        }
        case ParseContext::SWAP: {
            constructed.back() = MakeNodeRef<Key>(internal::NoDupCheck{}, Fragment::WRAP_S, Vector(std::move(constructed.back())));
            break;
        }
        case ParseContext::CHECK: {
            constructed.back() = MakeNodeRef<Key>(internal::NoDupCheck{}, Fragment::WRAP_C, Vector(std::move(constructed.back())));
            break;
        }
        case ParseContext::DUP_IF: {
            constructed.back() = MakeNodeRef<Key>(internal::NoDupCheck{}, Fragment::WRAP_D, Vector(std::move(constructed.back())));
            break;
        }
        case ParseContext::NON_ZERO: {
            constructed.back() = MakeNodeRef<Key>(internal::NoDupCheck{}, Fragment::WRAP_J, Vector(std::move(constructed.back())));
            break;
        }
        case ParseContext::ZERO_NOTEQUAL: {
            constructed.back() = MakeNodeRef<Key>(internal::NoDupCheck{}, Fragment::WRAP_N, Vector(std::move(constructed.back())));
            break;
        }
        case ParseContext::VERIFY: {
            script_size += (constructed.back()->GetType() << "x"_mst);
            constructed.back() = MakeNodeRef<Key>(internal::NoDupCheck{}, Fragment::WRAP_V, Vector(std::move(constructed.back())));
            break;
        }
        case ParseContext::WRAP_U: {
            constructed.back() = MakeNodeRef<Key>(internal::NoDupCheck{}, Fragment::OR_I, Vector(std::move(constructed.back()), MakeNodeRef<Key>(internal::NoDupCheck{}, Fragment::JUST_0)));
            break;
        }
        case ParseContext::WRAP_T: {
            constructed.back() = MakeNodeRef<Key>(internal::NoDupCheck{}, Fragment::AND_V, Vector(std::move(constructed.back()), MakeNodeRef<Key>(internal::NoDupCheck{}, Fragment::JUST_1)));
            break;
        }
        case ParseContext::AND_B: {
            BuildBack(Fragment::AND_B, constructed);
            break;
        }
        case ParseContext::AND_N: {
            auto mid = std::move(constructed.back());
            constructed.pop_back();
            constructed.back() = MakeNodeRef<Key>(internal::NoDupCheck{}, Fragment::ANDOR, Vector(std::move(constructed.back()), std::move(mid), MakeNodeRef<Key>(ctx, Fragment::JUST_0)));
            break;
        }
        case ParseContext::AND_V: {
            BuildBack(Fragment::AND_V, constructed);
            break;
        }
        case ParseContext::OR_B: {
            BuildBack(Fragment::OR_B, constructed);
            break;
        }
        case ParseContext::OR_C: {
            BuildBack(Fragment::OR_C, constructed);
            break;
        }
        case ParseContext::OR_D: {
            BuildBack(Fragment::OR_D, constructed);
            break;
        }
        case ParseContext::OR_I: {
            BuildBack(Fragment::OR_I, constructed);
            break;
        }
        case ParseContext::ANDOR: {
            auto right = std::move(constructed.back());
            constructed.pop_back();
            auto mid = std::move(constructed.back());
            constructed.pop_back();
            constructed.back() = MakeNodeRef<Key>(internal::NoDupCheck{}, Fragment::ANDOR, Vector(std::move(constructed.back()), std::move(mid), std::move(right)));
            break;
        }
        case ParseContext::THRESH: {
            if (in.size() < 1) return {};
            if (in[0] == ',') {
                in = in.subspan(1);
                to_parse.emplace_back(ParseContext::THRESH, n+1, k);
                to_parse.emplace_back(ParseContext::WRAPPED_EXPR, -1, -1);
                script_size += 2;
            } else if (in[0] == ')') {
                if (k > n) return {};
                in = in.subspan(1);
                std::vector<NodeRef<Key>> subs;
                for (int i = 0; i < n; ++i) {
                    subs.push_back(std::move(constructed.back()));
                    constructed.pop_back();
                }
                std::reverse(subs.begin(), subs.end());
                constructed.push_back(MakeNodeRef<Key>(internal::NoDupCheck{}, Fragment::THRESH, std::move(subs), k));
            } else {
                return {};
            }
            break;
        }
        case ParseContext::COMMA: {
            if (in.size() < 1 || in[0] != ',') return {};
            in = in.subspan(1);
            break;
        }
        case ParseContext::CLOSE_BRACKET: {
            if (in.size() < 1 || in[0] != ')') return {};
            in = in.subspan(1);
            break;
        }
        }
    }
    assert(constructed.size() == 1);
    assert(constructed[0]->ScriptSize() == script_size);
    if (in.size() > 0) return {};
    const NodeRef<Key> tl_node = std::move(constructed.front());
    tl_node->DuplicateKeyCheck(ctx);
    return tl_node;
}
std::optional<std::vector<Opcode>> DecomposeScript(const CScript& script);
std::optional<int64_t> ParseScriptNumber(const Opcode& in);
enum class DecodeContext {
    SINGLE_BKV_EXPR,
    BKV_EXPR,
    W_EXPR,
    SWAP,
    ALT,
    CHECK,
    DUP_IF,
    VERIFY,
    NON_ZERO,
    ZERO_NOTEQUAL,
    MAYBE_AND_V,
    AND_V,
    AND_B,
    ANDOR,
    OR_B,
    OR_C,
    OR_D,
    THRESH_W,
    THRESH_E,
    ENDIF,
    ENDIF_NOTIF,
    ENDIF_ELSE,
};
template<typename Key, typename Ctx, typename I>
inline NodeRef<Key> DecodeScript(I& in, I last, const Ctx& ctx)
{
    std::vector<std::tuple<DecodeContext, int64_t, int64_t>> to_parse;
    std::vector<NodeRef<Key>> constructed;
    to_parse.emplace_back(DecodeContext::BKV_EXPR, -1, -1);
    while (!to_parse.empty()) {
        if (!constructed.empty() && !constructed.back()->IsValid()) return {};
        auto [cur_context, n, k] = to_parse.back();
        to_parse.pop_back();
        switch(cur_context) {
        case DecodeContext::SINGLE_BKV_EXPR: {
            if (in >= last) return {};
            if (in[0].first == OP_1) {
                ++in;
                constructed.push_back(MakeNodeRef<Key>(internal::NoDupCheck{}, Fragment::JUST_1));
                break;
            }
            if (in[0].first == OP_0) {
                ++in;
                constructed.push_back(MakeNodeRef<Key>(internal::NoDupCheck{}, Fragment::JUST_0));
                break;
            }
            if (in[0].second.size() == 33) {
                auto key = ctx.FromPKBytes(in[0].second.begin(), in[0].second.end());
                if (!key) return {};
                ++in;
                constructed.push_back(MakeNodeRef<Key>(internal::NoDupCheck{}, Fragment::PK_K, Vector(std::move(*key))));
                break;
            }
            if (last - in >= 5 && in[0].first == OP_VERIFY && in[1].first == OP_EQUAL && in[3].first == OP_HASH160 && in[4].first == OP_DUP && in[2].second.size() == 20) {
                auto key = ctx.FromPKHBytes(in[2].second.begin(), in[2].second.end());
                if (!key) return {};
                in += 5;
                constructed.push_back(MakeNodeRef<Key>(internal::NoDupCheck{}, Fragment::PK_H, Vector(std::move(*key))));
                break;
            }
            std::optional<int64_t> num;
            if (last - in >= 2 && in[0].first == OP_CHECKSEQUENCEVERIFY && (num = ParseScriptNumber(in[1]))) {
                in += 2;
                if (*num < 1 || *num > 0x7FFFFFFFL) return {};
                constructed.push_back(MakeNodeRef<Key>(internal::NoDupCheck{}, Fragment::OLDER, *num));
                break;
            }
            if (last - in >= 2 && in[0].first == OP_CHECKLOCKTIMEVERIFY && (num = ParseScriptNumber(in[1]))) {
                in += 2;
                if (num < 1 || num > 0x7FFFFFFFL) return {};
                constructed.push_back(MakeNodeRef<Key>(internal::NoDupCheck{}, Fragment::AFTER, *num));
                break;
            }
            if (last - in >= 7 && in[0].first == OP_EQUAL && in[3].first == OP_VERIFY && in[4].first == OP_EQUAL && (num = ParseScriptNumber(in[5])) && num == 32 && in[6].first == OP_SIZE) {
                if (in[2].first == OP_SHA256 && in[1].second.size() == 32) {
                    constructed.push_back(MakeNodeRef<Key>(internal::NoDupCheck{}, Fragment::SHA256, in[1].second));
                    in += 7;
                    break;
                } else if (in[2].first == OP_RIPEMD160 && in[1].second.size() == 20) {
                    constructed.push_back(MakeNodeRef<Key>(internal::NoDupCheck{}, Fragment::RIPEMD160, in[1].second));
                    in += 7;
                    break;
                } else if (in[2].first == OP_HASH256 && in[1].second.size() == 32) {
                    constructed.push_back(MakeNodeRef<Key>(internal::NoDupCheck{}, Fragment::HASH256, in[1].second));
                    in += 7;
                    break;
                } else if (in[2].first == OP_HASH160 && in[1].second.size() == 20) {
                    constructed.push_back(MakeNodeRef<Key>(internal::NoDupCheck{}, Fragment::HASH160, in[1].second));
                    in += 7;
                    break;
                }
            }
            if (last - in >= 3 && in[0].first == OP_CHECKMULTISIG) {
                std::vector<Key> keys;
                const auto n = ParseScriptNumber(in[1]);
                if (!n || last - in < 3 + *n) return {};
                if (*n < 1 || *n > 20) return {};
                for (int i = 0; i < *n; ++i) {
                    if (in[2 + i].second.size() != 33) return {};
                    auto key = ctx.FromPKBytes(in[2 + i].second.begin(), in[2 + i].second.end());
                    if (!key) return {};
                    keys.push_back(std::move(*key));
                }
                const auto k = ParseScriptNumber(in[2 + *n]);
                if (!k || *k < 1 || *k > *n) return {};
                in += 3 + *n;
                std::reverse(keys.begin(), keys.end());
                constructed.push_back(MakeNodeRef<Key>(internal::NoDupCheck{}, Fragment::MULTI, std::move(keys), *k));
                break;
            }
            if (in[0].first == OP_CHECKSIG) {
                ++in;
                to_parse.emplace_back(DecodeContext::CHECK, -1, -1);
                to_parse.emplace_back(DecodeContext::SINGLE_BKV_EXPR, -1, -1);
                break;
            }
            if (in[0].first == OP_VERIFY) {
                ++in;
                to_parse.emplace_back(DecodeContext::VERIFY, -1, -1);
                to_parse.emplace_back(DecodeContext::SINGLE_BKV_EXPR, -1, -1);
                break;
            }
            if (in[0].first == OP_0NOTEQUAL) {
                ++in;
                to_parse.emplace_back(DecodeContext::ZERO_NOTEQUAL, -1, -1);
                to_parse.emplace_back(DecodeContext::SINGLE_BKV_EXPR, -1, -1);
                break;
            }
            if (last - in >= 3 && in[0].first == OP_EQUAL && (num = ParseScriptNumber(in[1]))) {
                if (*num < 1) return {};
                in += 2;
                to_parse.emplace_back(DecodeContext::THRESH_W, 0, *num);
                break;
            }
            if (in[0].first == OP_ENDIF) {
                ++in;
                to_parse.emplace_back(DecodeContext::ENDIF, -1, -1);
                to_parse.emplace_back(DecodeContext::BKV_EXPR, -1, -1);
                break;
            }
            if (in[0].first == OP_BOOLAND) {
                ++in;
                to_parse.emplace_back(DecodeContext::AND_B, -1, -1);
                to_parse.emplace_back(DecodeContext::SINGLE_BKV_EXPR, -1, -1);
                to_parse.emplace_back(DecodeContext::W_EXPR, -1, -1);
                break;
            }
            if (in[0].first == OP_BOOLOR) {
                ++in;
                to_parse.emplace_back(DecodeContext::OR_B, -1, -1);
                to_parse.emplace_back(DecodeContext::SINGLE_BKV_EXPR, -1, -1);
                to_parse.emplace_back(DecodeContext::W_EXPR, -1, -1);
                break;
            }
            return {};
        }
        case DecodeContext::BKV_EXPR: {
            to_parse.emplace_back(DecodeContext::MAYBE_AND_V, -1, -1);
            to_parse.emplace_back(DecodeContext::SINGLE_BKV_EXPR, -1, -1);
            break;
        }
        case DecodeContext::W_EXPR: {
            if (in >= last) return {};
            if (in[0].first == OP_FROMALTSTACK) {
                ++in;
                to_parse.emplace_back(DecodeContext::ALT, -1, -1);
            } else {
                to_parse.emplace_back(DecodeContext::SWAP, -1, -1);
            }
            to_parse.emplace_back(DecodeContext::BKV_EXPR, -1, -1);
            break;
        }
        case DecodeContext::MAYBE_AND_V: {
            if (in < last && in[0].first != OP_IF && in[0].first != OP_ELSE && in[0].first != OP_NOTIF && in[0].first != OP_TOALTSTACK && in[0].first != OP_SWAP) {
                to_parse.emplace_back(DecodeContext::AND_V, -1, -1);
                to_parse.emplace_back(DecodeContext::BKV_EXPR, -1, -1);
            }
            break;
        }
        case DecodeContext::SWAP: {
            if (in >= last || in[0].first != OP_SWAP || constructed.empty()) return {};
            ++in;
            constructed.back() = MakeNodeRef<Key>(internal::NoDupCheck{}, Fragment::WRAP_S, Vector(std::move(constructed.back())));
            break;
        }
        case DecodeContext::ALT: {
            if (in >= last || in[0].first != OP_TOALTSTACK || constructed.empty()) return {};
            ++in;
            constructed.back() = MakeNodeRef<Key>(internal::NoDupCheck{}, Fragment::WRAP_A, Vector(std::move(constructed.back())));
            break;
        }
        case DecodeContext::CHECK: {
            if (constructed.empty()) return {};
            constructed.back() = MakeNodeRef<Key>(internal::NoDupCheck{}, Fragment::WRAP_C, Vector(std::move(constructed.back())));
            break;
        }
        case DecodeContext::DUP_IF: {
            if (constructed.empty()) return {};
            constructed.back() = MakeNodeRef<Key>(internal::NoDupCheck{}, Fragment::WRAP_D, Vector(std::move(constructed.back())));
            break;
        }
        case DecodeContext::VERIFY: {
            if (constructed.empty()) return {};
            constructed.back() = MakeNodeRef<Key>(internal::NoDupCheck{}, Fragment::WRAP_V, Vector(std::move(constructed.back())));
            break;
        }
        case DecodeContext::NON_ZERO: {
            if (constructed.empty()) return {};
            constructed.back() = MakeNodeRef<Key>(internal::NoDupCheck{}, Fragment::WRAP_J, Vector(std::move(constructed.back())));
            break;
        }
        case DecodeContext::ZERO_NOTEQUAL: {
            if (constructed.empty()) return {};
            constructed.back() = MakeNodeRef<Key>(internal::NoDupCheck{}, Fragment::WRAP_N, Vector(std::move(constructed.back())));
            break;
        }
        case DecodeContext::AND_V: {
            if (constructed.size() < 2) return {};
            BuildBack(Fragment::AND_V, constructed,  true);
            break;
        }
        case DecodeContext::AND_B: {
            if (constructed.size() < 2) return {};
            BuildBack(Fragment::AND_B, constructed,  true);
            break;
        }
        case DecodeContext::OR_B: {
            if (constructed.size() < 2) return {};
            BuildBack(Fragment::OR_B, constructed,  true);
            break;
        }
        case DecodeContext::OR_C: {
            if (constructed.size() < 2) return {};
            BuildBack(Fragment::OR_C, constructed,  true);
            break;
        }
        case DecodeContext::OR_D: {
            if (constructed.size() < 2) return {};
            BuildBack(Fragment::OR_D, constructed,  true);
            break;
        }
        case DecodeContext::ANDOR: {
            if (constructed.size() < 3) return {};
            NodeRef<Key> left = std::move(constructed.back());
            constructed.pop_back();
            NodeRef<Key> right = std::move(constructed.back());
            constructed.pop_back();
            NodeRef<Key> mid = std::move(constructed.back());
            constructed.back() = MakeNodeRef<Key>(internal::NoDupCheck{}, Fragment::ANDOR, Vector(std::move(left), std::move(mid), std::move(right)));
            break;
        }
        case DecodeContext::THRESH_W: {
            if (in >= last) return {};
            if (in[0].first == OP_ADD) {
                ++in;
                to_parse.emplace_back(DecodeContext::THRESH_W, n+1, k);
                to_parse.emplace_back(DecodeContext::W_EXPR, -1, -1);
            } else {
                to_parse.emplace_back(DecodeContext::THRESH_E, n+1, k);
                to_parse.emplace_back(DecodeContext::SINGLE_BKV_EXPR, -1, -1);
            }
            break;
        }
        case DecodeContext::THRESH_E: {
            if (k < 1 || k > n || constructed.size() < static_cast<size_t>(n)) return {};
            std::vector<NodeRef<Key>> subs;
            for (int i = 0; i < n; ++i) {
                NodeRef<Key> sub = std::move(constructed.back());
                constructed.pop_back();
                subs.push_back(std::move(sub));
            }
            constructed.push_back(MakeNodeRef<Key>(internal::NoDupCheck{}, Fragment::THRESH, std::move(subs), k));
            break;
        }
        case DecodeContext::ENDIF: {
            if (in >= last) return {};
            if (in[0].first == OP_ELSE) {
                ++in;
                to_parse.emplace_back(DecodeContext::ENDIF_ELSE, -1, -1);
                to_parse.emplace_back(DecodeContext::BKV_EXPR, -1, -1);
            }
            else if (in[0].first == OP_IF) {
                if (last - in >= 2 && in[1].first == OP_DUP) {
                    in += 2;
                    to_parse.emplace_back(DecodeContext::DUP_IF, -1, -1);
                } else if (last - in >= 3 && in[1].first == OP_0NOTEQUAL && in[2].first == OP_SIZE) {
                    in += 3;
                    to_parse.emplace_back(DecodeContext::NON_ZERO, -1, -1);
                }
                else {
                    return {};
                }
            } else if (in[0].first == OP_NOTIF) {
                ++in;
                to_parse.emplace_back(DecodeContext::ENDIF_NOTIF, -1, -1);
            }
            else {
                return {};
            }
            break;
        }
        case DecodeContext::ENDIF_NOTIF: {
            if (in >= last) return {};
            if (in[0].first == OP_IFDUP) {
                ++in;
                to_parse.emplace_back(DecodeContext::OR_D, -1, -1);
            } else {
                to_parse.emplace_back(DecodeContext::OR_C, -1, -1);
            }
            to_parse.emplace_back(DecodeContext::SINGLE_BKV_EXPR, -1, -1);
            break;
        }
        case DecodeContext::ENDIF_ELSE: {
            if (in >= last) return {};
            if (in[0].first == OP_IF) {
                ++in;
                BuildBack(Fragment::OR_I, constructed,  true);
            } else if (in[0].first == OP_NOTIF) {
                ++in;
                to_parse.emplace_back(DecodeContext::ANDOR, -1, -1);
                to_parse.emplace_back(DecodeContext::SINGLE_BKV_EXPR, -1, -1);
            } else {
                return {};
            }
            break;
        }
        }
    }
    if (constructed.size() != 1) return {};
    const NodeRef<Key> tl_node = std::move(constructed.front());
    tl_node->DuplicateKeyCheck(ctx);
    if (!tl_node->IsValidTopLevel()) return {};
    return tl_node;
}
}
template<typename Ctx>
inline NodeRef<typename Ctx::Key> FromString(const std::string& str, const Ctx& ctx) {
    return internal::Parse<typename Ctx::Key>(str, ctx);
}
template<typename Ctx>
inline NodeRef<typename Ctx::Key> FromScript(const CScript& script, const Ctx& ctx) {
    using namespace internal;
    if (script.size() > MAX_STANDARD_P2WSH_SCRIPT_SIZE) return {};
    auto decomposed = DecomposeScript(script);
    if (!decomposed) return {};
    auto it = decomposed->begin();
    auto ret = DecodeScript<typename Ctx::Key>(it, decomposed->end(), ctx);
    if (!ret) return {};
    if (it != decomposed->end()) return {};
    return ret;
}
}
#endif
