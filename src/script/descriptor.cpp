#include <script/descriptor.h>
#include <key_io.h>
#include <pubkey.h>
#include <script/miniscript.h>
#include <script/script.h>
#include <script/standard.h>
#include <span.h>
#include <util/bip32.h>
#include <util/spanparsing.h>
#include <util/system.h>
#include <util/strencodings.h>
#include <util/vector.h>
#include <memory>
#include <optional>
#include <string>
#include <vector>
namespace {
uint64_t PolyMod(uint64_t c, int val)
{
    uint8_t c0 = c >> 35;
    c = ((c & 0x7ffffffff) << 5) ^ val;
    if (c0 & 1) c ^= 0xf5dee51989;
    if (c0 & 2) c ^= 0xa9fdca3312;
    if (c0 & 4) c ^= 0x1bab10e32d;
    if (c0 & 8) c ^= 0x3706b1677a;
    if (c0 & 16) c ^= 0x644d626ffd;
    return c;
}
std::string DescriptorChecksum(const Span<const char>& span)
{
    static std::string INPUT_CHARSET =
        "0123456789()[],'/*abcdefgh@:$%{}"
        "IJKLMNOPQRSTUVWXYZ&+-.;<=>?!^_|~"
        "ijklmnopqrstuvwxyzABCDEFGH`#\"\\ ";
    static std::string CHECKSUM_CHARSET = "qpzry9x8gf2tvdw0s3jn54khce6mua7l";
    uint64_t c = 1;
    int cls = 0;
    int clscount = 0;
    for (auto ch : span) {
        auto pos = INPUT_CHARSET.find(ch);
        if (pos == std::string::npos) return "";
        c = PolyMod(c, pos & 31);
        cls = cls * 3 + (pos >> 5);
        if (++clscount == 3) {
            c = PolyMod(c, cls);
            cls = 0;
            clscount = 0;
        }
    }
    if (clscount > 0) c = PolyMod(c, cls);
    for (int j = 0; j < 8; ++j) c = PolyMod(c, 0);
    c ^= 1;
    std::string ret(8, ' ');
    for (int j = 0; j < 8; ++j) ret[j] = CHECKSUM_CHARSET[(c >> (5 * (7 - j))) & 31];
    return ret;
}
std::string AddChecksum(const std::string& str) { return str + "#" + DescriptorChecksum(str); }
typedef std::vector<uint32_t> KeyPath;
struct PubkeyProvider
{
protected:
    uint32_t m_expr_index;
public:
    explicit PubkeyProvider(uint32_t exp_index) : m_expr_index(exp_index) {}
    virtual ~PubkeyProvider() = default;
    bool operator<(PubkeyProvider& other) const {
        CPubKey a, b;
        SigningProvider dummy;
        KeyOriginInfo dummy_info;
        GetPubKey(0, dummy, a, dummy_info);
        other.GetPubKey(0, dummy, b, dummy_info);
        return a < b;
    }
    virtual bool GetPubKey(int pos, const SigningProvider& arg, CPubKey& key, KeyOriginInfo& info, const DescriptorCache* read_cache = nullptr, DescriptorCache* write_cache = nullptr) const = 0;
    virtual bool IsRange() const = 0;
    virtual size_t GetSize() const = 0;
    virtual std::string ToString() const = 0;
    virtual bool ToPrivateString(const SigningProvider& arg, std::string& out) const = 0;
    virtual bool ToNormalizedString(const SigningProvider& arg, std::string& out, const DescriptorCache* cache = nullptr) const = 0;
    virtual bool GetPrivKey(int pos, const SigningProvider& arg, CKey& key) const = 0;
};
class OriginPubkeyProvider final : public PubkeyProvider
{
    KeyOriginInfo m_origin;
    std::unique_ptr<PubkeyProvider> m_provider;
    std::string OriginString() const
    {
        return HexStr(m_origin.fingerprint) + FormatHDKeypath(m_origin.path);
    }
public:
    OriginPubkeyProvider(uint32_t exp_index, KeyOriginInfo info, std::unique_ptr<PubkeyProvider> provider) : PubkeyProvider(exp_index), m_origin(std::move(info)), m_provider(std::move(provider)) {}
    bool GetPubKey(int pos, const SigningProvider& arg, CPubKey& key, KeyOriginInfo& info, const DescriptorCache* read_cache = nullptr, DescriptorCache* write_cache = nullptr) const override
    {
        if (!m_provider->GetPubKey(pos, arg, key, info, read_cache, write_cache)) return false;
        std::copy(std::begin(m_origin.fingerprint), std::end(m_origin.fingerprint), info.fingerprint);
        info.path.insert(info.path.begin(), m_origin.path.begin(), m_origin.path.end());
        return true;
    }
    bool IsRange() const override { return m_provider->IsRange(); }
    size_t GetSize() const override { return m_provider->GetSize(); }
    std::string ToString() const override { return "[" + OriginString() + "]" + m_provider->ToString(); }
    bool ToPrivateString(const SigningProvider& arg, std::string& ret) const override
    {
        std::string sub;
        if (!m_provider->ToPrivateString(arg, sub)) return false;
        ret = "[" + OriginString() + "]" + std::move(sub);
        return true;
    }
    bool ToNormalizedString(const SigningProvider& arg, std::string& ret, const DescriptorCache* cache) const override
    {
        std::string sub;
        if (!m_provider->ToNormalizedString(arg, sub, cache)) return false;
        if (sub[0] == '[') {
            sub = sub.substr(9);
            ret = "[" + OriginString() + std::move(sub);
        } else {
            ret = "[" + OriginString() + "]" + std::move(sub);
        }
        return true;
    }
    bool GetPrivKey(int pos, const SigningProvider& arg, CKey& key) const override
    {
        return m_provider->GetPrivKey(pos, arg, key);
    }
};
class ConstPubkeyProvider final : public PubkeyProvider
{
    CPubKey m_pubkey;
    bool m_xonly;
public:
    ConstPubkeyProvider(uint32_t exp_index, const CPubKey& pubkey, bool xonly) : PubkeyProvider(exp_index), m_pubkey(pubkey), m_xonly(xonly) {}
    bool GetPubKey(int pos, const SigningProvider& arg, CPubKey& key, KeyOriginInfo& info, const DescriptorCache* read_cache = nullptr, DescriptorCache* write_cache = nullptr) const override
    {
        key = m_pubkey;
        info.path.clear();
        CKeyID keyid = m_pubkey.GetID();
        std::copy(keyid.begin(), keyid.begin() + sizeof(info.fingerprint), info.fingerprint);
        return true;
    }
    bool IsRange() const override { return false; }
    size_t GetSize() const override { return m_pubkey.size(); }
    std::string ToString() const override { return m_xonly ? HexStr(m_pubkey).substr(2) : HexStr(m_pubkey); }
    bool ToPrivateString(const SigningProvider& arg, std::string& ret) const override
    {
        CKey key;
        if (m_xonly) {
            for (const auto& keyid : XOnlyPubKey(m_pubkey).GetKeyIDs()) {
                arg.GetKey(keyid, key);
                if (key.IsValid()) break;
            }
        } else {
            arg.GetKey(m_pubkey.GetID(), key);
        }
        if (!key.IsValid()) return false;
        ret = EncodeSecret(key);
        return true;
    }
    bool ToNormalizedString(const SigningProvider& arg, std::string& ret, const DescriptorCache* cache) const override
    {
        ret = ToString();
        return true;
    }
    bool GetPrivKey(int pos, const SigningProvider& arg, CKey& key) const override
    {
        return arg.GetKey(m_pubkey.GetID(), key);
    }
};
enum class DeriveType {
    NO,
    UNHARDENED,
    HARDENED,
};
class BIP32PubkeyProvider final : public PubkeyProvider
{
    CExtPubKey m_root_extkey;
    KeyPath m_path;
    DeriveType m_derive;
    bool GetExtKey(const SigningProvider& arg, CExtKey& ret) const
    {
        CKey key;
        if (!arg.GetKey(m_root_extkey.pubkey.GetID(), key)) return false;
        ret.nDepth = m_root_extkey.nDepth;
        std::copy(m_root_extkey.vchFingerprint, m_root_extkey.vchFingerprint + sizeof(ret.vchFingerprint), ret.vchFingerprint);
        ret.nChild = m_root_extkey.nChild;
        ret.chaincode = m_root_extkey.chaincode;
        ret.key = key;
        return true;
    }
    bool GetDerivedExtKey(const SigningProvider& arg, CExtKey& xprv, CExtKey& last_hardened) const
    {
        if (!GetExtKey(arg, xprv)) return false;
        for (auto entry : m_path) {
            if (!xprv.Derive(xprv, entry)) return false;
            if (entry >> 31) {
                last_hardened = xprv;
            }
        }
        return true;
    }
    bool IsHardened() const
    {
        if (m_derive == DeriveType::HARDENED) return true;
        for (auto entry : m_path) {
            if (entry >> 31) return true;
        }
        return false;
    }
public:
    BIP32PubkeyProvider(uint32_t exp_index, const CExtPubKey& extkey, KeyPath path, DeriveType derive) : PubkeyProvider(exp_index), m_root_extkey(extkey), m_path(std::move(path)), m_derive(derive) {}
    bool IsRange() const override { return m_derive != DeriveType::NO; }
    size_t GetSize() const override { return 33; }
    bool GetPubKey(int pos, const SigningProvider& arg, CPubKey& key_out, KeyOriginInfo& final_info_out, const DescriptorCache* read_cache = nullptr, DescriptorCache* write_cache = nullptr) const override
    {
        KeyOriginInfo parent_info;
        CKeyID keyid = m_root_extkey.pubkey.GetID();
        std::copy(keyid.begin(), keyid.begin() + sizeof(parent_info.fingerprint), parent_info.fingerprint);
        parent_info.path = m_path;
        KeyOriginInfo final_info_out_tmp = parent_info;
        if (m_derive == DeriveType::UNHARDENED) final_info_out_tmp.path.push_back((uint32_t)pos);
        if (m_derive == DeriveType::HARDENED) final_info_out_tmp.path.push_back(((uint32_t)pos) | 0x80000000L);
        CExtPubKey final_extkey = m_root_extkey;
        CExtPubKey parent_extkey = m_root_extkey;
        CExtPubKey last_hardened_extkey;
        bool der = true;
        if (read_cache) {
            if (!read_cache->GetCachedDerivedExtPubKey(m_expr_index, pos, final_extkey)) {
                if (m_derive == DeriveType::HARDENED) return false;
                if (!read_cache->GetCachedParentExtPubKey(m_expr_index, parent_extkey)) return false;
                final_extkey = parent_extkey;
                if (m_derive == DeriveType::UNHARDENED) der = parent_extkey.Derive(final_extkey, pos);
            }
        } else if (IsHardened()) {
            CExtKey xprv;
            CExtKey lh_xprv;
            if (!GetDerivedExtKey(arg, xprv, lh_xprv)) return false;
            parent_extkey = xprv.Neuter();
            if (m_derive == DeriveType::UNHARDENED) der = xprv.Derive(xprv, pos);
            if (m_derive == DeriveType::HARDENED) der = xprv.Derive(xprv, pos | 0x80000000UL);
            final_extkey = xprv.Neuter();
            if (lh_xprv.key.IsValid()) {
                last_hardened_extkey = lh_xprv.Neuter();
            }
        } else {
            for (auto entry : m_path) {
                if (!parent_extkey.Derive(parent_extkey, entry)) return false;
            }
            final_extkey = parent_extkey;
            if (m_derive == DeriveType::UNHARDENED) der = parent_extkey.Derive(final_extkey, pos);
            assert(m_derive != DeriveType::HARDENED);
        }
        if (!der) return false;
        final_info_out = final_info_out_tmp;
        key_out = final_extkey.pubkey;
        if (write_cache) {
            if (m_derive != DeriveType::HARDENED) {
                write_cache->CacheParentExtPubKey(m_expr_index, parent_extkey);
                if (last_hardened_extkey.pubkey.IsValid()) {
                    write_cache->CacheLastHardenedExtPubKey(m_expr_index, last_hardened_extkey);
                }
            } else if (final_info_out.path.size() > 0) {
                write_cache->CacheDerivedExtPubKey(m_expr_index, pos, final_extkey);
            }
        }
        return true;
    }
    std::string ToString() const override
    {
        std::string ret = EncodeExtPubKey(m_root_extkey) + FormatHDKeypath(m_path);
        if (IsRange()) {
            ret += "/*";
            if (m_derive == DeriveType::HARDENED) ret += '\'';
        }
        return ret;
    }
    bool ToPrivateString(const SigningProvider& arg, std::string& out) const override
    {
        CExtKey key;
        if (!GetExtKey(arg, key)) return false;
        out = EncodeExtKey(key) + FormatHDKeypath(m_path);
        if (IsRange()) {
            out += "/*";
            if (m_derive == DeriveType::HARDENED) out += '\'';
        }
        return true;
    }
    bool ToNormalizedString(const SigningProvider& arg, std::string& out, const DescriptorCache* cache) const override
    {
        if (m_derive == DeriveType::HARDENED) {
            out = ToString();
            return true;
        }
        int i = (int)m_path.size() - 1;
        for (; i >= 0; --i) {
            if (m_path.at(i) >> 31) {
                break;
            }
        }
        if (i == -1) {
            out = ToString();
            return true;
        }
        KeyOriginInfo origin;
        int k = 0;
        for (; k <= i; ++k) {
            origin.path.push_back(m_path.at(k));
        }
        KeyPath end_path;
        for (; k < (int)m_path.size(); ++k) {
            end_path.push_back(m_path.at(k));
        }
        CKeyID id = m_root_extkey.pubkey.GetID();
        std::copy(id.begin(), id.begin() + 4, origin.fingerprint);
        CExtPubKey xpub;
        CExtKey lh_xprv;
        if (cache != nullptr) {
            cache->GetCachedLastHardenedExtPubKey(m_expr_index, xpub);
        }
        if (!xpub.pubkey.IsValid()) {
            CExtKey xprv;
            if (!GetDerivedExtKey(arg, xprv, lh_xprv)) return false;
            xpub = lh_xprv.Neuter();
        }
        assert(xpub.pubkey.IsValid());
        std::string origin_str = HexStr(origin.fingerprint) + FormatHDKeypath(origin.path);
        out = "[" + origin_str + "]" + EncodeExtPubKey(xpub) + FormatHDKeypath(end_path);
        if (IsRange()) {
            out += "/*";
            assert(m_derive == DeriveType::UNHARDENED);
        }
        return true;
    }
    bool GetPrivKey(int pos, const SigningProvider& arg, CKey& key) const override
    {
        CExtKey extkey;
        CExtKey dummy;
        if (!GetDerivedExtKey(arg, extkey, dummy)) return false;
        if (m_derive == DeriveType::UNHARDENED && !extkey.Derive(extkey, pos)) return false;
        if (m_derive == DeriveType::HARDENED && !extkey.Derive(extkey, pos | 0x80000000UL)) return false;
        key = extkey.key;
        return true;
    }
};
class DescriptorImpl : public Descriptor
{
protected:
    const std::vector<std::unique_ptr<PubkeyProvider>> m_pubkey_args;
    const std::string m_name;
    const std::vector<std::unique_ptr<DescriptorImpl>> m_subdescriptor_args;
    virtual std::string ToStringExtra() const { return ""; }
    virtual std::vector<CScript> MakeScripts(const std::vector<CPubKey>& pubkeys, Span<const CScript> scripts, FlatSigningProvider& out) const = 0;
public:
    DescriptorImpl(std::vector<std::unique_ptr<PubkeyProvider>> pubkeys, const std::string& name) : m_pubkey_args(std::move(pubkeys)), m_name(name), m_subdescriptor_args() {}
    DescriptorImpl(std::vector<std::unique_ptr<PubkeyProvider>> pubkeys, std::unique_ptr<DescriptorImpl> script, const std::string& name) : m_pubkey_args(std::move(pubkeys)), m_name(name), m_subdescriptor_args(Vector(std::move(script))) {}
    DescriptorImpl(std::vector<std::unique_ptr<PubkeyProvider>> pubkeys, std::vector<std::unique_ptr<DescriptorImpl>> scripts, const std::string& name) : m_pubkey_args(std::move(pubkeys)), m_name(name), m_subdescriptor_args(std::move(scripts)) {}
    enum class StringType
    {
        PUBLIC,
        PRIVATE,
        NORMALIZED,
    };
    bool IsSolvable() const override
    {
        for (const auto& arg : m_subdescriptor_args) {
            if (!arg->IsSolvable()) return false;
        }
        return true;
    }
    bool IsRange() const final
    {
        for (const auto& pubkey : m_pubkey_args) {
            if (pubkey->IsRange()) return true;
        }
        for (const auto& arg : m_subdescriptor_args) {
            if (arg->IsRange()) return true;
        }
        return false;
    }
    virtual bool ToStringSubScriptHelper(const SigningProvider* arg, std::string& ret, const StringType type, const DescriptorCache* cache = nullptr) const
    {
        size_t pos = 0;
        for (const auto& scriptarg : m_subdescriptor_args) {
            if (pos++) ret += ",";
            std::string tmp;
            if (!scriptarg->ToStringHelper(arg, tmp, type, cache)) return false;
            ret += tmp;
        }
        return true;
    }
    virtual bool ToStringHelper(const SigningProvider* arg, std::string& out, const StringType type, const DescriptorCache* cache = nullptr) const
    {
        std::string extra = ToStringExtra();
        size_t pos = extra.size() > 0 ? 1 : 0;
        std::string ret = m_name + "(" + extra;
        for (const auto& pubkey : m_pubkey_args) {
            if (pos++) ret += ",";
            std::string tmp;
            switch (type) {
                case StringType::NORMALIZED:
                    if (!pubkey->ToNormalizedString(*arg, tmp, cache)) return false;
                    break;
                case StringType::PRIVATE:
                    if (!pubkey->ToPrivateString(*arg, tmp)) return false;
                    break;
                case StringType::PUBLIC:
                    tmp = pubkey->ToString();
                    break;
            }
            ret += tmp;
        }
        std::string subscript;
        if (!ToStringSubScriptHelper(arg, subscript, type, cache)) return false;
        if (pos && subscript.size()) ret += ',';
        out = std::move(ret) + std::move(subscript) + ")";
        return true;
    }
    std::string ToString() const final
    {
        std::string ret;
        ToStringHelper(nullptr, ret, StringType::PUBLIC);
        return AddChecksum(ret);
    }
    bool ToPrivateString(const SigningProvider& arg, std::string& out) const override
    {
        bool ret = ToStringHelper(&arg, out, StringType::PRIVATE);
        out = AddChecksum(out);
        return ret;
    }
    bool ToNormalizedString(const SigningProvider& arg, std::string& out, const DescriptorCache* cache) const override final
    {
        bool ret = ToStringHelper(&arg, out, StringType::NORMALIZED, cache);
        out = AddChecksum(out);
        return ret;
    }
    bool ExpandHelper(int pos, const SigningProvider& arg, const DescriptorCache* read_cache, std::vector<CScript>& output_scripts, FlatSigningProvider& out, DescriptorCache* write_cache) const
    {
        std::vector<std::pair<CPubKey, KeyOriginInfo>> entries;
        entries.reserve(m_pubkey_args.size());
        for (const auto& p : m_pubkey_args) {
            entries.emplace_back();
            if (!p->GetPubKey(pos, arg, entries.back().first, entries.back().second, read_cache, write_cache)) return false;
        }
        std::vector<CScript> subscripts;
        FlatSigningProvider subprovider;
        for (const auto& subarg : m_subdescriptor_args) {
            std::vector<CScript> outscripts;
            if (!subarg->ExpandHelper(pos, arg, read_cache, outscripts, subprovider, write_cache)) return false;
            assert(outscripts.size() == 1);
            subscripts.emplace_back(std::move(outscripts[0]));
        }
        out.Merge(std::move(subprovider));
        std::vector<CPubKey> pubkeys;
        pubkeys.reserve(entries.size());
        for (auto& entry : entries) {
            pubkeys.push_back(entry.first);
            out.origins.emplace(entry.first.GetID(), std::make_pair<CPubKey, KeyOriginInfo>(CPubKey(entry.first), std::move(entry.second)));
        }
        output_scripts = MakeScripts(pubkeys, Span{subscripts}, out);
        return true;
    }
    bool Expand(int pos, const SigningProvider& provider, std::vector<CScript>& output_scripts, FlatSigningProvider& out, DescriptorCache* write_cache = nullptr) const final
    {
        return ExpandHelper(pos, provider, nullptr, output_scripts, out, write_cache);
    }
    bool ExpandFromCache(int pos, const DescriptorCache& read_cache, std::vector<CScript>& output_scripts, FlatSigningProvider& out) const final
    {
        return ExpandHelper(pos, DUMMY_SIGNING_PROVIDER, &read_cache, output_scripts, out, nullptr);
    }
    void ExpandPrivate(int pos, const SigningProvider& provider, FlatSigningProvider& out) const final
    {
        for (const auto& p : m_pubkey_args) {
            CKey key;
            if (!p->GetPrivKey(pos, provider, key)) continue;
            out.keys.emplace(key.GetPubKey().GetID(), key);
        }
        for (const auto& arg : m_subdescriptor_args) {
            arg->ExpandPrivate(pos, provider, out);
        }
    }
    std::optional<OutputType> GetOutputType() const override { return std::nullopt; }
};
class AddressDescriptor final : public DescriptorImpl
{
    const CTxDestination m_destination;
protected:
    std::string ToStringExtra() const override { return EncodeDestination(m_destination); }
    std::vector<CScript> MakeScripts(const std::vector<CPubKey>&, Span<const CScript>, FlatSigningProvider&) const override { return Vector(GetScriptForDestination(m_destination)); }
public:
    AddressDescriptor(CTxDestination destination) : DescriptorImpl({}, "addr"), m_destination(std::move(destination)) {}
    bool IsSolvable() const final { return false; }
    std::optional<OutputType> GetOutputType() const override
    {
        return OutputTypeFromDestination(m_destination);
    }
    bool IsSingleType() const final { return true; }
    bool ToPrivateString(const SigningProvider& arg, std::string& out) const final { return false; }
};
class RawDescriptor final : public DescriptorImpl
{
    const CScript m_script;
protected:
    std::string ToStringExtra() const override { return HexStr(m_script); }
    std::vector<CScript> MakeScripts(const std::vector<CPubKey>&, Span<const CScript>, FlatSigningProvider&) const override { return Vector(m_script); }
public:
    RawDescriptor(CScript script) : DescriptorImpl({}, "raw"), m_script(std::move(script)) {}
    bool IsSolvable() const final { return false; }
    std::optional<OutputType> GetOutputType() const override
    {
        CTxDestination dest;
        ExtractDestination(m_script, dest);
        return OutputTypeFromDestination(dest);
    }
    bool IsSingleType() const final { return true; }
    bool ToPrivateString(const SigningProvider& arg, std::string& out) const final { return false; }
};
class PKDescriptor final : public DescriptorImpl
{
private:
    const bool m_xonly;
protected:
    std::vector<CScript> MakeScripts(const std::vector<CPubKey>& keys, Span<const CScript>, FlatSigningProvider&) const override
    {
        if (m_xonly) {
            CScript script = CScript() << ToByteVector(XOnlyPubKey(keys[0])) << OP_CHECKSIG;
            return Vector(std::move(script));
        } else {
            return Vector(GetScriptForRawPubKey(keys[0]));
        }
    }
public:
    PKDescriptor(std::unique_ptr<PubkeyProvider> prov, bool xonly = false) : DescriptorImpl(Vector(std::move(prov)), "pk"), m_xonly(xonly) {}
    bool IsSingleType() const final { return true; }
};
class PKHDescriptor final : public DescriptorImpl
{
protected:
    std::vector<CScript> MakeScripts(const std::vector<CPubKey>& keys, Span<const CScript>, FlatSigningProvider& out) const override
    {
        CKeyID id = keys[0].GetID();
        out.pubkeys.emplace(id, keys[0]);
        return Vector(GetScriptForDestination(PKHash(id)));
    }
public:
    PKHDescriptor(std::unique_ptr<PubkeyProvider> prov) : DescriptorImpl(Vector(std::move(prov)), "pkh") {}
    std::optional<OutputType> GetOutputType() const override { return OutputType::LEGACY; }
    bool IsSingleType() const final { return true; }
};
class WPKHDescriptor final : public DescriptorImpl
{
protected:
    std::vector<CScript> MakeScripts(const std::vector<CPubKey>& keys, Span<const CScript>, FlatSigningProvider& out) const override
    {
        CKeyID id = keys[0].GetID();
        out.pubkeys.emplace(id, keys[0]);
        return Vector(GetScriptForDestination(WitnessV0KeyHash(id)));
    }
public:
    WPKHDescriptor(std::unique_ptr<PubkeyProvider> prov) : DescriptorImpl(Vector(std::move(prov)), "wpkh") {}
    std::optional<OutputType> GetOutputType() const override { return OutputType::BECH32; }
    bool IsSingleType() const final { return true; }
};
class ComboDescriptor final : public DescriptorImpl
{
protected:
    std::vector<CScript> MakeScripts(const std::vector<CPubKey>& keys, Span<const CScript>, FlatSigningProvider& out) const override
    {
        std::vector<CScript> ret;
        CKeyID id = keys[0].GetID();
        out.pubkeys.emplace(id, keys[0]);
        ret.emplace_back(GetScriptForRawPubKey(keys[0]));
        ret.emplace_back(GetScriptForDestination(PKHash(id)));
        if (keys[0].IsCompressed()) {
            CScript p2wpkh = GetScriptForDestination(WitnessV0KeyHash(id));
            out.scripts.emplace(CScriptID(p2wpkh), p2wpkh);
            ret.emplace_back(p2wpkh);
            ret.emplace_back(GetScriptForDestination(ScriptHash(p2wpkh)));
        }
        return ret;
    }
public:
    ComboDescriptor(std::unique_ptr<PubkeyProvider> prov) : DescriptorImpl(Vector(std::move(prov)), "combo") {}
    bool IsSingleType() const final { return false; }
};
class MultisigDescriptor final : public DescriptorImpl
{
    const int m_threshold;
    const bool m_sorted;
protected:
    std::string ToStringExtra() const override { return strprintf("%i", m_threshold); }
    std::vector<CScript> MakeScripts(const std::vector<CPubKey>& keys, Span<const CScript>, FlatSigningProvider&) const override {
        if (m_sorted) {
            std::vector<CPubKey> sorted_keys(keys);
            std::sort(sorted_keys.begin(), sorted_keys.end());
            return Vector(GetScriptForMultisig(m_threshold, sorted_keys));
        }
        return Vector(GetScriptForMultisig(m_threshold, keys));
    }
public:
    MultisigDescriptor(int threshold, std::vector<std::unique_ptr<PubkeyProvider>> providers, bool sorted = false) : DescriptorImpl(std::move(providers), sorted ? "sortedmulti" : "multi"), m_threshold(threshold), m_sorted(sorted) {}
    bool IsSingleType() const final { return true; }
};
class MultiADescriptor final : public DescriptorImpl
{
    const int m_threshold;
    const bool m_sorted;
protected:
    std::string ToStringExtra() const override { return strprintf("%i", m_threshold); }
    std::vector<CScript> MakeScripts(const std::vector<CPubKey>& keys, Span<const CScript>, FlatSigningProvider&) const override {
        CScript ret;
        std::vector<XOnlyPubKey> xkeys;
        for (const auto& key : keys) xkeys.emplace_back(key);
        if (m_sorted) std::sort(xkeys.begin(), xkeys.end());
        ret << ToByteVector(xkeys[0]) << OP_CHECKSIG;
        for (size_t i = 1; i < keys.size(); ++i) {
            ret << ToByteVector(xkeys[i]) << OP_CHECKSIGADD;
        }
        ret << m_threshold << OP_NUMEQUAL;
        return Vector(std::move(ret));
    }
public:
    MultiADescriptor(int threshold, std::vector<std::unique_ptr<PubkeyProvider>> providers, bool sorted = false) : DescriptorImpl(std::move(providers), sorted ? "sortedmulti_a" : "multi_a"), m_threshold(threshold), m_sorted(sorted) {}
    bool IsSingleType() const final { return true; }
};
class SHDescriptor final : public DescriptorImpl
{
protected:
    std::vector<CScript> MakeScripts(const std::vector<CPubKey>&, Span<const CScript> scripts, FlatSigningProvider& out) const override
    {
        auto ret = Vector(GetScriptForDestination(ScriptHash(scripts[0])));
        if (ret.size()) out.scripts.emplace(CScriptID(scripts[0]), scripts[0]);
        return ret;
    }
public:
    SHDescriptor(std::unique_ptr<DescriptorImpl> desc) : DescriptorImpl({}, std::move(desc), "sh") {}
    std::optional<OutputType> GetOutputType() const override
    {
        assert(m_subdescriptor_args.size() == 1);
        if (m_subdescriptor_args[0]->GetOutputType() == OutputType::BECH32) return OutputType::P2SH_SEGWIT;
        return OutputType::LEGACY;
    }
    bool IsSingleType() const final { return true; }
};
class WSHDescriptor final : public DescriptorImpl
{
protected:
    std::vector<CScript> MakeScripts(const std::vector<CPubKey>&, Span<const CScript> scripts, FlatSigningProvider& out) const override
    {
        auto ret = Vector(GetScriptForDestination(WitnessV0ScriptHash(scripts[0])));
        if (ret.size()) out.scripts.emplace(CScriptID(scripts[0]), scripts[0]);
        return ret;
    }
public:
    WSHDescriptor(std::unique_ptr<DescriptorImpl> desc) : DescriptorImpl({}, std::move(desc), "wsh") {}
    std::optional<OutputType> GetOutputType() const override { return OutputType::BECH32; }
    bool IsSingleType() const final { return true; }
};
class TRDescriptor final : public DescriptorImpl
{
    std::vector<int> m_depths;
protected:
    std::vector<CScript> MakeScripts(const std::vector<CPubKey>& keys, Span<const CScript> scripts, FlatSigningProvider& out) const override
    {
        TaprootBuilder builder;
        assert(m_depths.size() == scripts.size());
        for (size_t pos = 0; pos < m_depths.size(); ++pos) {
            builder.Add(m_depths[pos], scripts[pos], TAPROOT_LEAF_TAPSCRIPT);
        }
        if (!builder.IsComplete()) return {};
        assert(keys.size() == 1);
        XOnlyPubKey xpk(keys[0]);
        if (!xpk.IsFullyValid()) return {};
        builder.Finalize(xpk);
        WitnessV1Taproot output = builder.GetOutput();
        out.tr_trees[output] = builder;
        out.pubkeys.emplace(keys[0].GetID(), keys[0]);
        return Vector(GetScriptForDestination(output));
    }
    bool ToStringSubScriptHelper(const SigningProvider* arg, std::string& ret, const StringType type, const DescriptorCache* cache = nullptr) const override
    {
        if (m_depths.empty()) return true;
        std::vector<bool> path;
        for (size_t pos = 0; pos < m_depths.size(); ++pos) {
            if (pos) ret += ',';
            while ((int)path.size() <= m_depths[pos]) {
                if (path.size()) ret += '{';
                path.push_back(false);
            }
            std::string tmp;
            if (!m_subdescriptor_args[pos]->ToStringHelper(arg, tmp, type, cache)) return false;
            ret += tmp;
            while (!path.empty() && path.back()) {
                if (path.size() > 1) ret += '}';
                path.pop_back();
            }
            if (!path.empty()) path.back() = true;
        }
        return true;
    }
public:
    TRDescriptor(std::unique_ptr<PubkeyProvider> internal_key, std::vector<std::unique_ptr<DescriptorImpl>> descs, std::vector<int> depths) :
        DescriptorImpl(Vector(std::move(internal_key)), std::move(descs), "tr"), m_depths(std::move(depths))
    {
        assert(m_subdescriptor_args.size() == m_depths.size());
    }
    std::optional<OutputType> GetOutputType() const override { return OutputType::BECH32M; }
    bool IsSingleType() const final { return true; }
};
class ScriptMaker {
    const std::vector<CPubKey>& m_keys;
public:
    ScriptMaker(const std::vector<CPubKey>& keys LIFETIMEBOUND) : m_keys(keys) {}
    std::vector<unsigned char> ToPKBytes(uint32_t key) const {
        return {m_keys[key].begin(), m_keys[key].end()};
    }
    std::vector<unsigned char> ToPKHBytes(uint32_t key) const {
        auto id = m_keys[key].GetID();
        return {id.begin(), id.end()};
    }
};
class StringMaker {
    const SigningProvider* m_arg;
    const std::vector<std::unique_ptr<PubkeyProvider>>& m_pubkeys;
    bool m_private;
public:
    StringMaker(const SigningProvider* arg LIFETIMEBOUND, const std::vector<std::unique_ptr<PubkeyProvider>>& pubkeys LIFETIMEBOUND, bool priv)
        : m_arg(arg), m_pubkeys(pubkeys), m_private(priv) {}
    std::optional<std::string> ToString(uint32_t key) const
    {
        std::string ret;
        if (m_private) {
            if (!m_pubkeys[key]->ToPrivateString(*m_arg, ret)) return {};
        } else {
            ret = m_pubkeys[key]->ToString();
        }
        return ret;
    }
};
class MiniscriptDescriptor final : public DescriptorImpl
{
private:
    miniscript::NodeRef<uint32_t> m_node;
protected:
    std::vector<CScript> MakeScripts(const std::vector<CPubKey>& keys, Span<const CScript> scripts,
                                     FlatSigningProvider& provider) const override
    {
        for (const auto& key : keys) provider.pubkeys.emplace(key.GetID(), key);
        return Vector(m_node->ToScript(ScriptMaker(keys)));
    }
public:
    MiniscriptDescriptor(std::vector<std::unique_ptr<PubkeyProvider>> providers, miniscript::NodeRef<uint32_t> node)
        : DescriptorImpl(std::move(providers), "?"), m_node(std::move(node)) {}
    bool ToStringHelper(const SigningProvider* arg, std::string& out, const StringType type,
                        const DescriptorCache* cache = nullptr) const override
    {
        if (const auto res = m_node->ToString(StringMaker(arg, m_pubkey_args, type == StringType::PRIVATE))) {
            out = *res;
            return true;
        }
        return false;
    }
    bool IsSolvable() const override { return false; }
    bool IsSingleType() const final { return true; }
};
class RawTRDescriptor final : public DescriptorImpl
{
protected:
    std::vector<CScript> MakeScripts(const std::vector<CPubKey>& keys, Span<const CScript> scripts, FlatSigningProvider& out) const override
    {
        assert(keys.size() == 1);
        XOnlyPubKey xpk(keys[0]);
        if (!xpk.IsFullyValid()) return {};
        WitnessV1Taproot output{xpk};
        return Vector(GetScriptForDestination(output));
    }
public:
    RawTRDescriptor(std::unique_ptr<PubkeyProvider> output_key) : DescriptorImpl(Vector(std::move(output_key)), "rawtr") {}
    std::optional<OutputType> GetOutputType() const override { return OutputType::BECH32M; }
    bool IsSingleType() const final { return true; }
};
enum class ParseScriptContext {
    TOP,
    P2SH,
    P2WPKH,
    P2WSH,
    P2TR,
};
[[nodiscard]] bool ParseKeyPath(const std::vector<Span<const char>>& split, KeyPath& out, std::string& error)
{
    for (size_t i = 1; i < split.size(); ++i) {
        Span<const char> elem = split[i];
        bool hardened = false;
        if (elem.size() > 0 && (elem[elem.size() - 1] == '\'' || elem[elem.size() - 1] == 'h')) {
            elem = elem.first(elem.size() - 1);
            hardened = true;
        }
        uint32_t p;
        if (!ParseUInt32(std::string(elem.begin(), elem.end()), &p)) {
            error = strprintf("Key path value '%s' is not a valid uint32", std::string(elem.begin(), elem.end()));
            return false;
        } else if (p > 0x7FFFFFFFUL) {
            error = strprintf("Key path value %u is out of range", p);
            return false;
        }
        out.push_back(p | (((uint32_t)hardened) << 31));
    }
    return true;
}
std::unique_ptr<PubkeyProvider> ParsePubkeyInner(uint32_t key_exp_index, const Span<const char>& sp, ParseScriptContext ctx, FlatSigningProvider& out, std::string& error)
{
    using namespace spanparsing;
    bool permit_uncompressed = ctx == ParseScriptContext::TOP || ctx == ParseScriptContext::P2SH;
    auto split = Split(sp, '/');
    std::string str(split[0].begin(), split[0].end());
    if (str.size() == 0) {
        error = "No key provided";
        return nullptr;
    }
    if (split.size() == 1) {
        if (IsHex(str)) {
            std::vector<unsigned char> data = ParseHex(str);
            CPubKey pubkey(data);
            if (pubkey.IsFullyValid()) {
                if (permit_uncompressed || pubkey.IsCompressed()) {
                    return std::make_unique<ConstPubkeyProvider>(key_exp_index, pubkey, false);
                } else {
                    error = "Uncompressed keys are not allowed";
                    return nullptr;
                }
            } else if (data.size() == 32 && ctx == ParseScriptContext::P2TR) {
                unsigned char fullkey[33] = {0x02};
                std::copy(data.begin(), data.end(), fullkey + 1);
                pubkey.Set(std::begin(fullkey), std::end(fullkey));
                if (pubkey.IsFullyValid()) {
                    return std::make_unique<ConstPubkeyProvider>(key_exp_index, pubkey, true);
                }
            }
            error = strprintf("Pubkey '%s' is invalid", str);
            return nullptr;
        }
        CKey key = DecodeSecret(str);
        if (key.IsValid()) {
            if (permit_uncompressed || key.IsCompressed()) {
                CPubKey pubkey = key.GetPubKey();
                out.keys.emplace(pubkey.GetID(), key);
                return std::make_unique<ConstPubkeyProvider>(key_exp_index, pubkey, ctx == ParseScriptContext::P2TR);
            } else {
                error = "Uncompressed keys are not allowed";
                return nullptr;
            }
        }
    }
    CExtKey extkey = DecodeExtKey(str);
    CExtPubKey extpubkey = DecodeExtPubKey(str);
    if (!extkey.key.IsValid() && !extpubkey.pubkey.IsValid()) {
        error = strprintf("key '%s' is not valid", str);
        return nullptr;
    }
    KeyPath path;
    DeriveType type = DeriveType::NO;
    if (split.back() == Span{"*"}.first(1)) {
        split.pop_back();
        type = DeriveType::UNHARDENED;
    } else if (split.back() == Span{"*'"}.first(2) || split.back() == Span{"*h"}.first(2)) {
        split.pop_back();
        type = DeriveType::HARDENED;
    }
    if (!ParseKeyPath(split, path, error)) return nullptr;
    if (extkey.key.IsValid()) {
        extpubkey = extkey.Neuter();
        out.keys.emplace(extpubkey.pubkey.GetID(), extkey.key);
    }
    return std::make_unique<BIP32PubkeyProvider>(key_exp_index, extpubkey, std::move(path), type);
}
std::unique_ptr<PubkeyProvider> ParsePubkey(uint32_t key_exp_index, const Span<const char>& sp, ParseScriptContext ctx, FlatSigningProvider& out, std::string& error)
{
    using namespace spanparsing;
    auto origin_split = Split(sp, ']');
    if (origin_split.size() > 2) {
        error = "Multiple ']' characters found for a single pubkey";
        return nullptr;
    }
    if (origin_split.size() == 1) return ParsePubkeyInner(key_exp_index, origin_split[0], ctx, out, error);
    if (origin_split[0].empty() || origin_split[0][0] != '[') {
        error = strprintf("Key origin start '[ character expected but not found, got '%c' instead",
                          origin_split[0].empty() ?   ']' : origin_split[0][0]);
        return nullptr;
    }
    auto slash_split = Split(origin_split[0].subspan(1), '/');
    if (slash_split[0].size() != 8) {
        error = strprintf("Fingerprint is not 4 bytes (%u characters instead of 8 characters)", slash_split[0].size());
        return nullptr;
    }
    std::string fpr_hex = std::string(slash_split[0].begin(), slash_split[0].end());
    if (!IsHex(fpr_hex)) {
        error = strprintf("Fingerprint '%s' is not hex", fpr_hex);
        return nullptr;
    }
    auto fpr_bytes = ParseHex(fpr_hex);
    KeyOriginInfo info;
    static_assert(sizeof(info.fingerprint) == 4, "Fingerprint must be 4 bytes");
    assert(fpr_bytes.size() == 4);
    std::copy(fpr_bytes.begin(), fpr_bytes.end(), info.fingerprint);
    if (!ParseKeyPath(slash_split, info.path, error)) return nullptr;
    auto provider = ParsePubkeyInner(key_exp_index, origin_split[1], ctx, out, error);
    if (!provider) return nullptr;
    return std::make_unique<OriginPubkeyProvider>(key_exp_index, std::move(info), std::move(provider));
}
std::unique_ptr<PubkeyProvider> InferPubkey(const CPubKey& pubkey, ParseScriptContext, const SigningProvider& provider)
{
    std::unique_ptr<PubkeyProvider> key_provider = std::make_unique<ConstPubkeyProvider>(0, pubkey, false);
    KeyOriginInfo info;
    if (provider.GetKeyOrigin(pubkey.GetID(), info)) {
        return std::make_unique<OriginPubkeyProvider>(0, std::move(info), std::move(key_provider));
    }
    return key_provider;
}
std::unique_ptr<PubkeyProvider> InferXOnlyPubkey(const XOnlyPubKey& xkey, ParseScriptContext ctx, const SigningProvider& provider)
{
    unsigned char full_key[CPubKey::COMPRESSED_SIZE] = {0x02};
    std::copy(xkey.begin(), xkey.end(), full_key + 1);
    CPubKey pubkey(full_key);
    std::unique_ptr<PubkeyProvider> key_provider = std::make_unique<ConstPubkeyProvider>(0, pubkey, true);
    KeyOriginInfo info;
    if (provider.GetKeyOriginByXOnly(xkey, info)) {
        return std::make_unique<OriginPubkeyProvider>(0, std::move(info), std::move(key_provider));
    }
    return key_provider;
}
struct KeyParser {
    using Key = uint32_t;
    FlatSigningProvider* m_out;
    const SigningProvider* m_in;
    mutable std::vector<std::unique_ptr<PubkeyProvider>> m_keys;
    mutable std::string m_key_parsing_error;
    KeyParser(FlatSigningProvider* out LIFETIMEBOUND, const SigningProvider* in LIFETIMEBOUND) : m_out(out), m_in(in) {}
    bool KeyCompare(const Key& a, const Key& b) const {
        return *m_keys.at(a) < *m_keys.at(b);
    }
    template<typename I> std::optional<Key> FromString(I begin, I end) const
    {
        assert(m_out);
        Key key = m_keys.size();
        auto pk = ParsePubkey(key, {&*begin, &*end}, ParseScriptContext::P2WSH, *m_out, m_key_parsing_error);
        if (!pk) return {};
        m_keys.push_back(std::move(pk));
        return key;
    }
    std::optional<std::string> ToString(const Key& key) const
    {
        return m_keys.at(key)->ToString();
    }
    template<typename I> std::optional<Key> FromPKBytes(I begin, I end) const
    {
        assert(m_in);
        CPubKey pubkey(begin, end);
        if (pubkey.IsValid()) {
            Key key = m_keys.size();
            m_keys.push_back(InferPubkey(pubkey, ParseScriptContext::P2WSH, *m_in));
            return key;
        }
        return {};
    }
    template<typename I> std::optional<Key> FromPKHBytes(I begin, I end) const
    {
        assert(end - begin == 20);
        assert(m_in);
        uint160 hash;
        std::copy(begin, end, hash.begin());
        CKeyID keyid(hash);
        CPubKey pubkey;
        if (m_in->GetPubKey(keyid, pubkey)) {
            Key key = m_keys.size();
            m_keys.push_back(InferPubkey(pubkey, ParseScriptContext::P2WSH, *m_in));
            return key;
        }
        return {};
    }
};
std::unique_ptr<DescriptorImpl> ParseScript(uint32_t& key_exp_index, Span<const char>& sp, ParseScriptContext ctx, FlatSigningProvider& out, std::string& error)
{
    using namespace spanparsing;
    auto expr = Expr(sp);
    if (Func("pk", expr)) {
        auto pubkey = ParsePubkey(key_exp_index, expr, ctx, out, error);
        if (!pubkey) {
            error = strprintf("pk(): %s", error);
            return nullptr;
        }
        ++key_exp_index;
        return std::make_unique<PKDescriptor>(std::move(pubkey), ctx == ParseScriptContext::P2TR);
    }
    if ((ctx == ParseScriptContext::TOP || ctx == ParseScriptContext::P2SH || ctx == ParseScriptContext::P2WSH) && Func("pkh", expr)) {
        auto pubkey = ParsePubkey(key_exp_index, expr, ctx, out, error);
        if (!pubkey) {
            error = strprintf("pkh(): %s", error);
            return nullptr;
        }
        ++key_exp_index;
        return std::make_unique<PKHDescriptor>(std::move(pubkey));
    } else if (Func("pkh", expr)) {
        error = "Can only have pkh at top level, in sh(), or in wsh()";
        return nullptr;
    }
    if (ctx == ParseScriptContext::TOP && Func("combo", expr)) {
        auto pubkey = ParsePubkey(key_exp_index, expr, ctx, out, error);
        if (!pubkey) {
            error = strprintf("combo(): %s", error);
            return nullptr;
        }
        ++key_exp_index;
        return std::make_unique<ComboDescriptor>(std::move(pubkey));
    } else if (Func("combo", expr)) {
        error = "Can only have combo() at top level";
        return nullptr;
    }
    const bool multi = Func("multi", expr);
    const bool sortedmulti = !multi && Func("sortedmulti", expr);
    const bool multi_a = !(multi || sortedmulti) && Func("multi_a", expr);
    const bool sortedmulti_a = !(multi || sortedmulti || multi_a) && Func("sortedmulti_a", expr);
    if (((ctx == ParseScriptContext::TOP || ctx == ParseScriptContext::P2SH || ctx == ParseScriptContext::P2WSH) && (multi || sortedmulti)) ||
        (ctx == ParseScriptContext::P2TR && (multi_a || sortedmulti_a))) {
        auto threshold = Expr(expr);
        uint32_t thres;
        std::vector<std::unique_ptr<PubkeyProvider>> providers;
        if (!ParseUInt32(std::string(threshold.begin(), threshold.end()), &thres)) {
            error = strprintf("Multi threshold '%s' is not valid", std::string(threshold.begin(), threshold.end()));
            return nullptr;
        }
        size_t script_size = 0;
        while (expr.size()) {
            if (!Const(",", expr)) {
                error = strprintf("Multi: expected ',', got '%c'", expr[0]);
                return nullptr;
            }
            auto arg = Expr(expr);
            auto pk = ParsePubkey(key_exp_index, arg, ctx, out, error);
            if (!pk) {
                error = strprintf("Multi: %s", error);
                return nullptr;
            }
            script_size += pk->GetSize() + 1;
            providers.emplace_back(std::move(pk));
            key_exp_index++;
        }
        if ((multi || sortedmulti) && (providers.empty() || providers.size() > MAX_PUBKEYS_PER_MULTISIG)) {
            error = strprintf("Cannot have %u keys in multisig; must have between 1 and %d keys, inclusive", providers.size(), MAX_PUBKEYS_PER_MULTISIG);
            return nullptr;
        } else if ((multi_a || sortedmulti_a) && (providers.empty() || providers.size() > MAX_PUBKEYS_PER_MULTI_A)) {
            error = strprintf("Cannot have %u keys in multi_a; must have between 1 and %d keys, inclusive", providers.size(), MAX_PUBKEYS_PER_MULTI_A);
            return nullptr;
        } else if (thres < 1) {
            error = strprintf("Multisig threshold cannot be %d, must be at least 1", thres);
            return nullptr;
        } else if (thres > providers.size()) {
            error = strprintf("Multisig threshold cannot be larger than the number of keys; threshold is %d but only %u keys specified", thres, providers.size());
            return nullptr;
        }
        if (ctx == ParseScriptContext::TOP) {
            if (providers.size() > 3) {
                error = strprintf("Cannot have %u pubkeys in bare multisig; only at most 3 pubkeys", providers.size());
                return nullptr;
            }
        }
        if (ctx == ParseScriptContext::P2SH) {
            if (script_size + 3 > MAX_SCRIPT_ELEMENT_SIZE) {
                error = strprintf("P2SH script is too large, %d bytes is larger than %d bytes", script_size + 3, MAX_SCRIPT_ELEMENT_SIZE);
                return nullptr;
            }
        }
        if (multi || sortedmulti) {
            return std::make_unique<MultisigDescriptor>(thres, std::move(providers), sortedmulti);
        } else {
            return std::make_unique<MultiADescriptor>(thres, std::move(providers), sortedmulti_a);
        }
    } else if (multi || sortedmulti) {
        error = "Can only have multi/sortedmulti at top level, in sh(), or in wsh()";
        return nullptr;
    } else if (multi_a || sortedmulti_a) {
        error = "Can only have multi_a/sortedmulti_a inside tr()";
        return nullptr;
    }
    if ((ctx == ParseScriptContext::TOP || ctx == ParseScriptContext::P2SH) && Func("wpkh", expr)) {
        auto pubkey = ParsePubkey(key_exp_index, expr, ParseScriptContext::P2WPKH, out, error);
        if (!pubkey) {
            error = strprintf("wpkh(): %s", error);
            return nullptr;
        }
        key_exp_index++;
        return std::make_unique<WPKHDescriptor>(std::move(pubkey));
    } else if (Func("wpkh", expr)) {
        error = "Can only have wpkh() at top level or inside sh()";
        return nullptr;
    }
    if (ctx == ParseScriptContext::TOP && Func("sh", expr)) {
        auto desc = ParseScript(key_exp_index, expr, ParseScriptContext::P2SH, out, error);
        if (!desc || expr.size()) return nullptr;
        return std::make_unique<SHDescriptor>(std::move(desc));
    } else if (Func("sh", expr)) {
        error = "Can only have sh() at top level";
        return nullptr;
    }
    if ((ctx == ParseScriptContext::TOP || ctx == ParseScriptContext::P2SH) && Func("wsh", expr)) {
        auto desc = ParseScript(key_exp_index, expr, ParseScriptContext::P2WSH, out, error);
        if (!desc || expr.size()) return nullptr;
        return std::make_unique<WSHDescriptor>(std::move(desc));
    } else if (Func("wsh", expr)) {
        error = "Can only have wsh() at top level or inside sh()";
        return nullptr;
    }
    if (ctx == ParseScriptContext::TOP && Func("addr", expr)) {
        CTxDestination dest = DecodeDestination(std::string(expr.begin(), expr.end()));
        if (!IsValidDestination(dest)) {
            error = "Address is not valid";
            return nullptr;
        }
        return std::make_unique<AddressDescriptor>(std::move(dest));
    } else if (Func("addr", expr)) {
        error = "Can only have addr() at top level";
        return nullptr;
    }
    if (ctx == ParseScriptContext::TOP && Func("tr", expr)) {
        auto arg = Expr(expr);
        auto internal_key = ParsePubkey(key_exp_index, arg, ParseScriptContext::P2TR, out, error);
        if (!internal_key) {
            error = strprintf("tr(): %s", error);
            return nullptr;
        }
        ++key_exp_index;
        std::vector<std::unique_ptr<DescriptorImpl>> subscripts;
        std::vector<int> depths;
        if (expr.size()) {
            if (!Const(",", expr)) {
                error = strprintf("tr: expected ',', got '%c'", expr[0]);
                return nullptr;
            }
            std::vector<bool> branches;
            do {
                while (Const("{", expr)) {
                    branches.push_back(false);
                    if (branches.size() > TAPROOT_CONTROL_MAX_NODE_COUNT) {
                        error = strprintf("tr() supports at most %i nesting levels", TAPROOT_CONTROL_MAX_NODE_COUNT);
                        return nullptr;
                    }
                }
                auto sarg = Expr(expr);
                subscripts.emplace_back(ParseScript(key_exp_index, sarg, ParseScriptContext::P2TR, out, error));
                if (!subscripts.back()) return nullptr;
                depths.push_back(branches.size());
                while (branches.size() && branches.back()) {
                    if (!Const("}", expr)) {
                        error = strprintf("tr(): expected '}' after script expression");
                        return nullptr;
                    }
                    branches.pop_back();
                }
                if (branches.size() && !branches.back()) {
                    if (!Const(",", expr)) {
                        error = strprintf("tr(): expected ',' after script expression");
                        return nullptr;
                    }
                    branches.back() = true;
                }
            } while (branches.size());
            if (expr.size()) {
                error = strprintf("tr(): expected ')' after script expression");
                return nullptr;
            }
        }
        assert(TaprootBuilder::ValidDepths(depths));
        return std::make_unique<TRDescriptor>(std::move(internal_key), std::move(subscripts), std::move(depths));
    } else if (Func("tr", expr)) {
        error = "Can only have tr at top level";
        return nullptr;
    }
    if (ctx == ParseScriptContext::TOP && Func("rawtr", expr)) {
        auto arg = Expr(expr);
        if (expr.size()) {
            error = strprintf("rawtr(): only one key expected.");
            return nullptr;
        }
        auto output_key = ParsePubkey(key_exp_index, arg, ParseScriptContext::P2TR, out, error);
        if (!output_key) return nullptr;
        ++key_exp_index;
        return std::make_unique<RawTRDescriptor>(std::move(output_key));
    } else if (Func("rawtr", expr)) {
        error = "Can only have rawtr at top level";
        return nullptr;
    }
    if (ctx == ParseScriptContext::TOP && Func("raw", expr)) {
        std::string str(expr.begin(), expr.end());
        if (!IsHex(str)) {
            error = "Raw script is not hex";
            return nullptr;
        }
        auto bytes = ParseHex(str);
        return std::make_unique<RawDescriptor>(CScript(bytes.begin(), bytes.end()));
    } else if (Func("raw", expr)) {
        error = "Can only have raw() at top level";
        return nullptr;
    }
    {
        KeyParser parser(&out, nullptr);
        auto node = miniscript::FromString(std::string(expr.begin(), expr.end()), parser);
        if (node) {
            if (ctx != ParseScriptContext::P2WSH) {
                error = "Miniscript expressions can only be used in wsh";
                return nullptr;
            }
            if (parser.m_key_parsing_error != "") {
                error = std::move(parser.m_key_parsing_error);
                return nullptr;
            }
            if (!node->IsSane()) {
                auto insane_node = node.get();
                if (const auto sub = node->FindInsaneSub()) insane_node = sub;
                if (const auto str = insane_node->ToString(parser)) error = *str;
                if (!insane_node->IsValid()) {
                    error += " is invalid";
                } else {
                    error += " is not sane";
                    if (!insane_node->IsNonMalleable()) {
                        error += ": malleable witnesses exist";
                    } else if (insane_node == node.get() && !insane_node->NeedsSignature()) {
                        error += ": witnesses without signature exist";
                    } else if (!insane_node->CheckTimeLocksMix()) {
                        error += ": contains mixes of timelocks expressed in blocks and seconds";
                    } else if (!insane_node->CheckDuplicateKey()) {
                        error += ": contains duplicate public keys";
                    } else if (!insane_node->ValidSatisfactions()) {
                        error += ": needs witnesses that may exceed resource limits";
                    }
                }
                return nullptr;
            }
            return std::make_unique<MiniscriptDescriptor>(std::move(parser.m_keys), std::move(node));
        }
    }
    if (ctx == ParseScriptContext::P2SH) {
        error = "A function is needed within P2SH";
        return nullptr;
    } else if (ctx == ParseScriptContext::P2WSH) {
        error = "A function is needed within P2WSH";
        return nullptr;
    }
    error = strprintf("'%s' is not a valid descriptor function", std::string(expr.begin(), expr.end()));
    return nullptr;
}
std::unique_ptr<DescriptorImpl> InferMultiA(const CScript& script, ParseScriptContext ctx, const SigningProvider& provider)
{
    auto match = MatchMultiA(script);
    if (!match) return {};
    std::vector<std::unique_ptr<PubkeyProvider>> keys;
    keys.reserve(match->second.size());
    for (const auto keyspan : match->second) {
        if (keyspan.size() != 32) return {};
        auto key = InferXOnlyPubkey(XOnlyPubKey{keyspan}, ctx, provider);
        if (!key) return {};
        keys.push_back(std::move(key));
    }
    return std::make_unique<MultiADescriptor>(match->first, std::move(keys));
}
std::unique_ptr<DescriptorImpl> InferScript(const CScript& script, ParseScriptContext ctx, const SigningProvider& provider)
{
    if (ctx == ParseScriptContext::P2TR && script.size() == 34 && script[0] == 32 && script[33] == OP_CHECKSIG) {
        XOnlyPubKey key{Span{script}.subspan(1, 32)};
        return std::make_unique<PKDescriptor>(InferXOnlyPubkey(key, ctx, provider), true);
    }
    if (ctx == ParseScriptContext::P2TR) {
        auto ret = InferMultiA(script, ctx, provider);
        if (ret) return ret;
    }
    std::vector<std::vector<unsigned char>> data;
    TxoutType txntype = Solver(script, data);
    if (txntype == TxoutType::PUBKEY && (ctx == ParseScriptContext::TOP || ctx == ParseScriptContext::P2SH || ctx == ParseScriptContext::P2WSH)) {
        CPubKey pubkey(data[0]);
        if (pubkey.IsValid()) {
            return std::make_unique<PKDescriptor>(InferPubkey(pubkey, ctx, provider));
        }
    }
    if (txntype == TxoutType::PUBKEYHASH && (ctx == ParseScriptContext::TOP || ctx == ParseScriptContext::P2SH || ctx == ParseScriptContext::P2WSH)) {
        uint160 hash(data[0]);
        CKeyID keyid(hash);
        CPubKey pubkey;
        if (provider.GetPubKey(keyid, pubkey)) {
            return std::make_unique<PKHDescriptor>(InferPubkey(pubkey, ctx, provider));
        }
    }
    if (txntype == TxoutType::WITNESS_V0_KEYHASH && (ctx == ParseScriptContext::TOP || ctx == ParseScriptContext::P2SH)) {
        uint160 hash(data[0]);
        CKeyID keyid(hash);
        CPubKey pubkey;
        if (provider.GetPubKey(keyid, pubkey)) {
            return std::make_unique<WPKHDescriptor>(InferPubkey(pubkey, ctx, provider));
        }
    }
    if (txntype == TxoutType::MULTISIG && (ctx == ParseScriptContext::TOP || ctx == ParseScriptContext::P2SH || ctx == ParseScriptContext::P2WSH)) {
        std::vector<std::unique_ptr<PubkeyProvider>> providers;
        for (size_t i = 1; i + 1 < data.size(); ++i) {
            CPubKey pubkey(data[i]);
            providers.push_back(InferPubkey(pubkey, ctx, provider));
        }
        return std::make_unique<MultisigDescriptor>((int)data[0][0], std::move(providers));
    }
    if (txntype == TxoutType::SCRIPTHASH && ctx == ParseScriptContext::TOP) {
        uint160 hash(data[0]);
        CScriptID scriptid(hash);
        CScript subscript;
        if (provider.GetCScript(scriptid, subscript)) {
            auto sub = InferScript(subscript, ParseScriptContext::P2SH, provider);
            if (sub) return std::make_unique<SHDescriptor>(std::move(sub));
        }
    }
    if (txntype == TxoutType::WITNESS_V0_SCRIPTHASH && (ctx == ParseScriptContext::TOP || ctx == ParseScriptContext::P2SH)) {
        CScriptID scriptid;
        CRIPEMD160().Write(data[0].data(), data[0].size()).Finalize(scriptid.begin());
        CScript subscript;
        if (provider.GetCScript(scriptid, subscript)) {
            auto sub = InferScript(subscript, ParseScriptContext::P2WSH, provider);
            if (sub) return std::make_unique<WSHDescriptor>(std::move(sub));
        }
    }
    if (txntype == TxoutType::WITNESS_V1_TAPROOT && ctx == ParseScriptContext::TOP) {
        XOnlyPubKey pubkey;
        std::copy(data[0].begin(), data[0].end(), pubkey.begin());
        TaprootSpendData tap;
        if (provider.GetTaprootSpendData(pubkey, tap)) {
            auto tree = InferTaprootTree(tap, pubkey);
            if (tree) {
                bool ok = true;
                std::vector<std::unique_ptr<DescriptorImpl>> subscripts;
                std::vector<int> depths;
                for (const auto& [depth, script, leaf_ver] : *tree) {
                    std::unique_ptr<DescriptorImpl> subdesc;
                    if (leaf_ver == TAPROOT_LEAF_TAPSCRIPT) {
                        subdesc = InferScript(script, ParseScriptContext::P2TR, provider);
                    }
                    if (!subdesc) {
                        ok = false;
                        break;
                    } else {
                        subscripts.push_back(std::move(subdesc));
                        depths.push_back(depth);
                    }
                }
                if (ok) {
                    auto key = InferXOnlyPubkey(tap.internal_key, ParseScriptContext::P2TR, provider);
                    return std::make_unique<TRDescriptor>(std::move(key), std::move(subscripts), std::move(depths));
                }
            }
        }
        if (pubkey.IsFullyValid()) {
            auto key = InferXOnlyPubkey(pubkey, ParseScriptContext::P2TR, provider);
            if (key) {
                return std::make_unique<RawTRDescriptor>(std::move(key));
            }
        }
    }
    if (ctx == ParseScriptContext::P2WSH) {
        KeyParser parser(nullptr, &provider);
        auto node = miniscript::FromScript(script, parser);
        if (node && node->IsSane()) {
            return std::make_unique<MiniscriptDescriptor>(std::move(parser.m_keys), std::move(node));
        }
    }
    CTxDestination dest;
    if (ExtractDestination(script, dest)) {
        if (GetScriptForDestination(dest) == script) {
            return std::make_unique<AddressDescriptor>(std::move(dest));
        }
    }
    return std::make_unique<RawDescriptor>(script);
}
}
bool CheckChecksum(Span<const char>& sp, bool require_checksum, std::string& error, std::string* out_checksum = nullptr)
{
    using namespace spanparsing;
    auto check_split = Split(sp, '#');
    if (check_split.size() > 2) {
        error = "Multiple '#' symbols";
        return false;
    }
    if (check_split.size() == 1 && require_checksum){
        error = "Missing checksum";
        return false;
    }
    if (check_split.size() == 2) {
        if (check_split[1].size() != 8) {
            error = strprintf("Expected 8 character checksum, not %u characters", check_split[1].size());
            return false;
        }
    }
    auto checksum = DescriptorChecksum(check_split[0]);
    if (checksum.empty()) {
        error = "Invalid characters in payload";
        return false;
    }
    if (check_split.size() == 2) {
        if (!std::equal(checksum.begin(), checksum.end(), check_split[1].begin())) {
            error = strprintf("Provided checksum '%s' does not match computed checksum '%s'", std::string(check_split[1].begin(), check_split[1].end()), checksum);
            return false;
        }
    }
    if (out_checksum) *out_checksum = std::move(checksum);
    sp = check_split[0];
    return true;
}
std::unique_ptr<Descriptor> Parse(const std::string& descriptor, FlatSigningProvider& out, std::string& error, bool require_checksum)
{
    Span<const char> sp{descriptor};
    if (!CheckChecksum(sp, require_checksum, error)) return nullptr;
    uint32_t key_exp_index = 0;
    auto ret = ParseScript(key_exp_index, sp, ParseScriptContext::TOP, out, error);
    if (sp.size() == 0 && ret) return std::unique_ptr<Descriptor>(std::move(ret));
    return nullptr;
}
std::string GetDescriptorChecksum(const std::string& descriptor)
{
    std::string ret;
    std::string error;
    Span<const char> sp{descriptor};
    if (!CheckChecksum(sp, false, error, &ret)) return "";
    return ret;
}
std::unique_ptr<Descriptor> InferDescriptor(const CScript& script, const SigningProvider& provider)
{
    return InferScript(script, ParseScriptContext::TOP, provider);
}
void DescriptorCache::CacheParentExtPubKey(uint32_t key_exp_pos, const CExtPubKey& xpub)
{
    m_parent_xpubs[key_exp_pos] = xpub;
}
void DescriptorCache::CacheDerivedExtPubKey(uint32_t key_exp_pos, uint32_t der_index, const CExtPubKey& xpub)
{
    auto& xpubs = m_derived_xpubs[key_exp_pos];
    xpubs[der_index] = xpub;
}
void DescriptorCache::CacheLastHardenedExtPubKey(uint32_t key_exp_pos, const CExtPubKey& xpub)
{
    m_last_hardened_xpubs[key_exp_pos] = xpub;
}
bool DescriptorCache::GetCachedParentExtPubKey(uint32_t key_exp_pos, CExtPubKey& xpub) const
{
    const auto& it = m_parent_xpubs.find(key_exp_pos);
    if (it == m_parent_xpubs.end()) return false;
    xpub = it->second;
    return true;
}
bool DescriptorCache::GetCachedDerivedExtPubKey(uint32_t key_exp_pos, uint32_t der_index, CExtPubKey& xpub) const
{
    const auto& key_exp_it = m_derived_xpubs.find(key_exp_pos);
    if (key_exp_it == m_derived_xpubs.end()) return false;
    const auto& der_it = key_exp_it->second.find(der_index);
    if (der_it == key_exp_it->second.end()) return false;
    xpub = der_it->second;
    return true;
}
bool DescriptorCache::GetCachedLastHardenedExtPubKey(uint32_t key_exp_pos, CExtPubKey& xpub) const
{
    const auto& it = m_last_hardened_xpubs.find(key_exp_pos);
    if (it == m_last_hardened_xpubs.end()) return false;
    xpub = it->second;
    return true;
}
DescriptorCache DescriptorCache::MergeAndDiff(const DescriptorCache& other)
{
    DescriptorCache diff;
    for (const auto& parent_xpub_pair : other.GetCachedParentExtPubKeys()) {
        CExtPubKey xpub;
        if (GetCachedParentExtPubKey(parent_xpub_pair.first, xpub)) {
            if (xpub != parent_xpub_pair.second) {
                throw std::runtime_error(std::string(__func__) + ": New cached parent xpub does not match already cached parent xpub");
            }
            continue;
        }
        CacheParentExtPubKey(parent_xpub_pair.first, parent_xpub_pair.second);
        diff.CacheParentExtPubKey(parent_xpub_pair.first, parent_xpub_pair.second);
    }
    for (const auto& derived_xpub_map_pair : other.GetCachedDerivedExtPubKeys()) {
        for (const auto& derived_xpub_pair : derived_xpub_map_pair.second) {
            CExtPubKey xpub;
            if (GetCachedDerivedExtPubKey(derived_xpub_map_pair.first, derived_xpub_pair.first, xpub)) {
                if (xpub != derived_xpub_pair.second) {
                    throw std::runtime_error(std::string(__func__) + ": New cached derived xpub does not match already cached derived xpub");
                }
                continue;
            }
            CacheDerivedExtPubKey(derived_xpub_map_pair.first, derived_xpub_pair.first, derived_xpub_pair.second);
            diff.CacheDerivedExtPubKey(derived_xpub_map_pair.first, derived_xpub_pair.first, derived_xpub_pair.second);
        }
    }
    for (const auto& lh_xpub_pair : other.GetCachedLastHardenedExtPubKeys()) {
        CExtPubKey xpub;
        if (GetCachedLastHardenedExtPubKey(lh_xpub_pair.first, xpub)) {
            if (xpub != lh_xpub_pair.second) {
                throw std::runtime_error(std::string(__func__) + ": New cached last hardened xpub does not match already cached last hardened xpub");
            }
            continue;
        }
        CacheLastHardenedExtPubKey(lh_xpub_pair.first, lh_xpub_pair.second);
        diff.CacheLastHardenedExtPubKey(lh_xpub_pair.first, lh_xpub_pair.second);
    }
    return diff;
}
const ExtPubKeyMap DescriptorCache::GetCachedParentExtPubKeys() const
{
    return m_parent_xpubs;
}
const std::unordered_map<uint32_t, ExtPubKeyMap> DescriptorCache::GetCachedDerivedExtPubKeys() const
{
    return m_derived_xpubs;
}
const ExtPubKeyMap DescriptorCache::GetCachedLastHardenedExtPubKeys() const
{
    return m_last_hardened_xpubs;
}
