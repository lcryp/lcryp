#include <string>
#include <vector>
#include <script/script.h>
#include <script/standard.h>
#include <script/miniscript.h>
#include <assert.h>
namespace miniscript {
namespace internal {
Type SanitizeType(Type e) {
    int num_types = (e << "K"_mst) + (e << "V"_mst) + (e << "B"_mst) + (e << "W"_mst);
    if (num_types == 0) return ""_mst;
    assert(num_types == 1);
    assert(!(e << "z"_mst) || !(e << "o"_mst));
    assert(!(e << "n"_mst) || !(e << "z"_mst));
    assert(!(e << "n"_mst) || !(e << "W"_mst));
    assert(!(e << "V"_mst) || !(e << "d"_mst));
    assert(!(e << "K"_mst) ||  (e << "u"_mst));
    assert(!(e << "V"_mst) || !(e << "u"_mst));
    assert(!(e << "e"_mst) || !(e << "f"_mst));
    assert(!(e << "e"_mst) ||  (e << "d"_mst));
    assert(!(e << "V"_mst) || !(e << "e"_mst));
    assert(!(e << "d"_mst) || !(e << "f"_mst));
    assert(!(e << "V"_mst) ||  (e << "f"_mst));
    assert(!(e << "K"_mst) ||  (e << "s"_mst));
    assert(!(e << "z"_mst) ||  (e << "m"_mst));
    return e;
}
Type ComputeType(Fragment fragment, Type x, Type y, Type z, const std::vector<Type>& sub_types, uint32_t k, size_t data_size, size_t n_subs, size_t n_keys) {
    if (fragment == Fragment::SHA256 || fragment == Fragment::HASH256) {
        assert(data_size == 32);
    } else if (fragment == Fragment::RIPEMD160 || fragment == Fragment::HASH160) {
        assert(data_size == 20);
    } else {
        assert(data_size == 0);
    }
    if (fragment == Fragment::OLDER || fragment == Fragment::AFTER) {
        assert(k >= 1 && k < 0x80000000UL);
    } else if (fragment == Fragment::MULTI) {
        assert(k >= 1 && k <= n_keys);
    } else if (fragment == Fragment::THRESH) {
        assert(k >= 1 && k <= n_subs);
    } else {
        assert(k == 0);
    }
    if (fragment == Fragment::AND_V || fragment == Fragment::AND_B || fragment == Fragment::OR_B ||
        fragment == Fragment::OR_C || fragment == Fragment::OR_I || fragment == Fragment::OR_D) {
        assert(n_subs == 2);
    } else if (fragment == Fragment::ANDOR) {
        assert(n_subs == 3);
    } else if (fragment == Fragment::WRAP_A || fragment == Fragment::WRAP_S || fragment == Fragment::WRAP_C ||
               fragment == Fragment::WRAP_D || fragment == Fragment::WRAP_V || fragment == Fragment::WRAP_J ||
               fragment == Fragment::WRAP_N) {
        assert(n_subs == 1);
    } else if (fragment != Fragment::THRESH) {
        assert(n_subs == 0);
    }
    if (fragment == Fragment::PK_K || fragment == Fragment::PK_H) {
        assert(n_keys == 1);
    } else if (fragment == Fragment::MULTI) {
        assert(n_keys >= 1 && n_keys <= 20);
    } else {
        assert(n_keys == 0);
    }
    switch (fragment) {
        case Fragment::PK_K: return "Konudemsxk"_mst;
        case Fragment::PK_H: return "Knudemsxk"_mst;
        case Fragment::OLDER: return
            "g"_mst.If(k & CTxIn::SEQUENCE_LOCKTIME_TYPE_FLAG) |
            "h"_mst.If(!(k & CTxIn::SEQUENCE_LOCKTIME_TYPE_FLAG)) |
            "Bzfmxk"_mst;
        case Fragment::AFTER: return
            "i"_mst.If(k >= LOCKTIME_THRESHOLD) |
            "j"_mst.If(k < LOCKTIME_THRESHOLD) |
            "Bzfmxk"_mst;
        case Fragment::SHA256: return "Bonudmk"_mst;
        case Fragment::RIPEMD160: return "Bonudmk"_mst;
        case Fragment::HASH256: return "Bonudmk"_mst;
        case Fragment::HASH160: return "Bonudmk"_mst;
        case Fragment::JUST_1: return "Bzufmxk"_mst;
        case Fragment::JUST_0: return "Bzudemsxk"_mst;
        case Fragment::WRAP_A: return
            "W"_mst.If(x << "B"_mst) |
            (x & "ghijk"_mst) |
            (x & "udfems"_mst) |
            "x"_mst;
        case Fragment::WRAP_S: return
            "W"_mst.If(x << "Bo"_mst) |
            (x & "ghijk"_mst) |
            (x & "udfemsx"_mst);
        case Fragment::WRAP_C: return
            "B"_mst.If(x << "K"_mst) |
            (x & "ghijk"_mst) |
            (x & "ondfem"_mst) |
            "us"_mst;
        case Fragment::WRAP_D: return
            "B"_mst.If(x << "Vz"_mst) |
            "o"_mst.If(x << "z"_mst) |
            "e"_mst.If(x << "f"_mst) |
            (x & "ghijk"_mst) |
            (x & "ms"_mst) |
            "ndx"_mst;
        case Fragment::WRAP_V: return
            "V"_mst.If(x << "B"_mst) |
            (x & "ghijk"_mst) |
            (x & "zonms"_mst) |
            "fx"_mst;
        case Fragment::WRAP_J: return
            "B"_mst.If(x << "Bn"_mst) |
            "e"_mst.If(x << "f"_mst) |
            (x & "ghijk"_mst) |
            (x & "oums"_mst) |
            "ndx"_mst;
        case Fragment::WRAP_N: return
            (x & "ghijk"_mst) |
            (x & "Bzondfems"_mst) |
            "ux"_mst;
        case Fragment::AND_V: return
            (y & "KVB"_mst).If(x << "V"_mst) |
            (x & "n"_mst) | (y & "n"_mst).If(x << "z"_mst) |
            ((x | y) & "o"_mst).If((x | y) << "z"_mst) |
            (x & y & "dmz"_mst) |
            ((x | y) & "s"_mst) |
            "f"_mst.If((y << "f"_mst) || (x << "s"_mst)) |
            (y & "ux"_mst) |
            ((x | y) & "ghij"_mst) |
            "k"_mst.If(((x & y) << "k"_mst) &&
                !(((x << "g"_mst) && (y << "h"_mst)) ||
                ((x << "h"_mst) && (y << "g"_mst)) ||
                ((x << "i"_mst) && (y << "j"_mst)) ||
                ((x << "j"_mst) && (y << "i"_mst))));
        case Fragment::AND_B: return
            (x & "B"_mst).If(y << "W"_mst) |
            ((x | y) & "o"_mst).If((x | y) << "z"_mst) |
            (x & "n"_mst) | (y & "n"_mst).If(x << "z"_mst) |
            (x & y & "e"_mst).If((x & y) << "s"_mst) |
            (x & y & "dzm"_mst) |
            "f"_mst.If(((x & y) << "f"_mst) || (x << "sf"_mst) || (y << "sf"_mst)) |
            ((x | y) & "s"_mst) |
            "ux"_mst |
            ((x | y) & "ghij"_mst) |
            "k"_mst.If(((x & y) << "k"_mst) &&
                !(((x << "g"_mst) && (y << "h"_mst)) ||
                ((x << "h"_mst) && (y << "g"_mst)) ||
                ((x << "i"_mst) && (y << "j"_mst)) ||
                ((x << "j"_mst) && (y << "i"_mst))));
        case Fragment::OR_B: return
            "B"_mst.If(x << "Bd"_mst && y << "Wd"_mst) |
            ((x | y) & "o"_mst).If((x | y) << "z"_mst) |
            (x & y & "m"_mst).If((x | y) << "s"_mst && (x & y) << "e"_mst) |
            (x & y & "zse"_mst) |
            "dux"_mst |
            ((x | y) & "ghij"_mst) |
            (x & y & "k"_mst);
        case Fragment::OR_D: return
            (y & "B"_mst).If(x << "Bdu"_mst) |
            (x & "o"_mst).If(y << "z"_mst) |
            (x & y & "m"_mst).If(x << "e"_mst && (x | y) << "s"_mst) |
            (x & y & "zes"_mst) |
            (y & "ufd"_mst) |
            "x"_mst |
            ((x | y) & "ghij"_mst) |
            (x & y & "k"_mst);
        case Fragment::OR_C: return
            (y & "V"_mst).If(x << "Bdu"_mst) |
            (x & "o"_mst).If(y << "z"_mst) |
            (x & y & "m"_mst).If(x << "e"_mst && (x | y) << "s"_mst) |
            (x & y & "zs"_mst) |
            "fx"_mst |
            ((x | y) & "ghij"_mst) |
            (x & y & "k"_mst);
        case Fragment::OR_I: return
            (x & y & "VBKufs"_mst) |
            "o"_mst.If((x & y) << "z"_mst) |
            ((x | y) & "e"_mst).If((x | y) << "f"_mst) |
            (x & y & "m"_mst).If((x | y) << "s"_mst) |
            ((x | y) & "d"_mst) |
            "x"_mst |
            ((x | y) & "ghij"_mst) |
            (x & y & "k"_mst);
        case Fragment::ANDOR: return
            (y & z & "BKV"_mst).If(x << "Bdu"_mst) |
            (x & y & z & "z"_mst) |
            ((x | (y & z)) & "o"_mst).If((x | (y & z)) << "z"_mst) |
            (y & z & "u"_mst) |
            (z & "f"_mst).If((x << "s"_mst) || (y << "f"_mst)) |
            (z & "d"_mst) |
            (x & z & "e"_mst).If(x << "s"_mst || y << "f"_mst) |
            (x & y & z & "m"_mst).If(x << "e"_mst && (x | y | z) << "s"_mst) |
            (z & (x | y) & "s"_mst) |
            "x"_mst |
            ((x | y | z) & "ghij"_mst) |
            "k"_mst.If(((x & y & z) << "k"_mst) &&
                !(((x << "g"_mst) && (y << "h"_mst)) ||
                ((x << "h"_mst) && (y << "g"_mst)) ||
                ((x << "i"_mst) && (y << "j"_mst)) ||
                ((x << "j"_mst) && (y << "i"_mst))));
        case Fragment::MULTI: return "Bnudemsk"_mst;
        case Fragment::THRESH: {
            bool all_e = true;
            bool all_m = true;
            uint32_t args = 0;
            uint32_t num_s = 0;
            Type acc_tl = "k"_mst;
            for (size_t i = 0; i < sub_types.size(); ++i) {
                Type t = sub_types[i];
                if (!(t << (i ? "Wdu"_mst : "Bdu"_mst))) return ""_mst;
                if (!(t << "e"_mst)) all_e = false;
                if (!(t << "m"_mst)) all_m = false;
                if (t << "s"_mst) num_s += 1;
                args += (t << "z"_mst) ? 0 : (t << "o"_mst) ? 1 : 2;
                acc_tl = ((acc_tl | t) & "ghij"_mst) |
                    "k"_mst.If(((acc_tl & t) << "k"_mst) && ((k <= 1) ||
                        ((k > 1) && !(((acc_tl << "g"_mst) && (t << "h"_mst)) ||
                        ((acc_tl << "h"_mst) && (t << "g"_mst)) ||
                        ((acc_tl << "i"_mst) && (t << "j"_mst)) ||
                        ((acc_tl << "j"_mst) && (t << "i"_mst))))));
            }
            return "Bdu"_mst |
                   "z"_mst.If(args == 0) |
                   "o"_mst.If(args == 1) |
                   "e"_mst.If(all_e && num_s == n_subs) |
                   "m"_mst.If(all_e && all_m && num_s >= n_subs - k) |
                   "s"_mst.If(num_s >= n_subs - k + 1) |
                   acc_tl;
            }
    }
    assert(false);
}
size_t ComputeScriptLen(Fragment fragment, Type sub0typ, size_t subsize, uint32_t k, size_t n_subs, size_t n_keys) {
    switch (fragment) {
        case Fragment::JUST_1:
        case Fragment::JUST_0: return 1;
        case Fragment::PK_K: return 34;
        case Fragment::PK_H: return 3 + 21;
        case Fragment::OLDER:
        case Fragment::AFTER: return 1 + BuildScript(k).size();
        case Fragment::HASH256:
        case Fragment::SHA256: return 4 + 2 + 33;
        case Fragment::HASH160:
        case Fragment::RIPEMD160: return 4 + 2 + 21;
        case Fragment::MULTI: return 1 + BuildScript(n_keys).size() + BuildScript(k).size() + 34 * n_keys;
        case Fragment::AND_V: return subsize;
        case Fragment::WRAP_V: return subsize + (sub0typ << "x"_mst);
        case Fragment::WRAP_S:
        case Fragment::WRAP_C:
        case Fragment::WRAP_N:
        case Fragment::AND_B:
        case Fragment::OR_B: return subsize + 1;
        case Fragment::WRAP_A:
        case Fragment::OR_C: return subsize + 2;
        case Fragment::WRAP_D:
        case Fragment::OR_D:
        case Fragment::OR_I:
        case Fragment::ANDOR: return subsize + 3;
        case Fragment::WRAP_J: return subsize + 4;
        case Fragment::THRESH: return subsize + n_subs + BuildScript(k).size();
    }
    assert(false);
}
std::optional<std::vector<Opcode>> DecomposeScript(const CScript& script)
{
    std::vector<Opcode> out;
    CScript::const_iterator it = script.begin(), itend = script.end();
    while (it != itend) {
        std::vector<unsigned char> push_data;
        opcodetype opcode;
        if (!script.GetOp(it, opcode, push_data)) {
            return {};
        } else if (opcode >= OP_1 && opcode <= OP_16) {
            push_data.assign(1, CScript::DecodeOP_N(opcode));
        } else if (opcode == OP_CHECKSIGVERIFY) {
            out.emplace_back(OP_CHECKSIG, std::vector<unsigned char>());
            opcode = OP_VERIFY;
        } else if (opcode == OP_CHECKMULTISIGVERIFY) {
            out.emplace_back(OP_CHECKMULTISIG, std::vector<unsigned char>());
            opcode = OP_VERIFY;
        } else if (opcode == OP_EQUALVERIFY) {
            out.emplace_back(OP_EQUAL, std::vector<unsigned char>());
            opcode = OP_VERIFY;
        } else if (IsPushdataOp(opcode)) {
            if (!CheckMinimalPush(push_data, opcode)) return {};
        } else if (it != itend && (opcode == OP_CHECKSIG || opcode == OP_CHECKMULTISIG || opcode == OP_EQUAL) && (*it == OP_VERIFY)) {
            return {};
        }
        out.emplace_back(opcode, std::move(push_data));
    }
    std::reverse(out.begin(), out.end());
    return out;
}
std::optional<int64_t> ParseScriptNumber(const Opcode& in) {
    if (in.first == OP_0) {
        return 0;
    }
    if (!in.second.empty()) {
        if (IsPushdataOp(in.first) && !CheckMinimalPush(in.second, in.first)) return {};
        try {
            return CScriptNum(in.second, true).GetInt64();
        } catch(const scriptnum_error&) {}
    }
    return {};
}
int FindNextChar(Span<const char> sp, const char m)
{
    for (int i = 0; i < (int)sp.size(); ++i) {
        if (sp[i] == m) return i;
        if (sp[i] == ')') break;
    }
    return -1;
}
}
}
