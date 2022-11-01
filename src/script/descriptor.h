#ifndef LCRYP_SCRIPT_DESCRIPTOR_H
#define LCRYP_SCRIPT_DESCRIPTOR_H
#include <outputtype.h>
#include <script/script.h>
#include <script/sign.h>
#include <script/signingprovider.h>
#include <optional>
#include <vector>
using ExtPubKeyMap = std::unordered_map<uint32_t, CExtPubKey>;
class DescriptorCache {
private:
    std::unordered_map<uint32_t, ExtPubKeyMap> m_derived_xpubs;
    ExtPubKeyMap m_parent_xpubs;
    ExtPubKeyMap m_last_hardened_xpubs;
public:
    void CacheParentExtPubKey(uint32_t key_exp_pos, const CExtPubKey& xpub);
    bool GetCachedParentExtPubKey(uint32_t key_exp_pos, CExtPubKey& xpub) const;
    void CacheDerivedExtPubKey(uint32_t key_exp_pos, uint32_t der_index, const CExtPubKey& xpub);
    bool GetCachedDerivedExtPubKey(uint32_t key_exp_pos, uint32_t der_index, CExtPubKey& xpub) const;
    void CacheLastHardenedExtPubKey(uint32_t key_exp_pos, const CExtPubKey& xpub);
    bool GetCachedLastHardenedExtPubKey(uint32_t key_exp_pos, CExtPubKey& xpub) const;
    const ExtPubKeyMap GetCachedParentExtPubKeys() const;
    const std::unordered_map<uint32_t, ExtPubKeyMap> GetCachedDerivedExtPubKeys() const;
    const ExtPubKeyMap GetCachedLastHardenedExtPubKeys() const;
    DescriptorCache MergeAndDiff(const DescriptorCache& other);
};
struct Descriptor {
    virtual ~Descriptor() = default;
    virtual bool IsRange() const = 0;
    virtual bool IsSolvable() const = 0;
    virtual std::string ToString() const = 0;
    virtual bool IsSingleType() const = 0;
    virtual bool ToPrivateString(const SigningProvider& provider, std::string& out) const = 0;
    virtual bool ToNormalizedString(const SigningProvider& provider, std::string& out, const DescriptorCache* cache = nullptr) const = 0;
    virtual bool Expand(int pos, const SigningProvider& provider, std::vector<CScript>& output_scripts, FlatSigningProvider& out, DescriptorCache* write_cache = nullptr) const = 0;
    virtual bool ExpandFromCache(int pos, const DescriptorCache& read_cache, std::vector<CScript>& output_scripts, FlatSigningProvider& out) const = 0;
    virtual void ExpandPrivate(int pos, const SigningProvider& provider, FlatSigningProvider& out) const = 0;
    virtual std::optional<OutputType> GetOutputType() const = 0;
};
std::unique_ptr<Descriptor> Parse(const std::string& descriptor, FlatSigningProvider& out, std::string& error, bool require_checksum = false);
std::string GetDescriptorChecksum(const std::string& descriptor);
std::unique_ptr<Descriptor> InferDescriptor(const CScript& script, const SigningProvider& provider);
#endif
