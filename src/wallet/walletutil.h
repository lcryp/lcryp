#ifndef LCRYP_WALLET_WALLETUTIL_H
#define LCRYP_WALLET_WALLETUTIL_H
#include <fs.h>
#include <script/descriptor.h>
#include <vector>
namespace wallet {
enum WalletFeature
{
    FEATURE_BASE = 10500,
    FEATURE_WALLETCRYPT = 40000,
    FEATURE_COMPRPUBKEY = 60000,
    FEATURE_HD = 130000,
    FEATURE_HD_SPLIT = 139900,
    FEATURE_NO_DEFAULT_KEY = 159900,
    FEATURE_PRE_SPLIT_KEYPOOL = 169900,
    FEATURE_LATEST = FEATURE_PRE_SPLIT_KEYPOOL
};
bool IsFeatureSupported(int wallet_version, int feature_version);
WalletFeature GetClosestWalletFeature(int version);
enum WalletFlags : uint64_t {
    WALLET_FLAG_AVOID_REUSE = (1ULL << 0),
    WALLET_FLAG_KEY_ORIGIN_METADATA = (1ULL << 1),
    WALLET_FLAG_LAST_HARDENED_XPUB_CACHED = (1ULL << 2),
    WALLET_FLAG_DISABLE_PRIVATE_KEYS = (1ULL << 32),
    WALLET_FLAG_BLANK_WALLET = (1ULL << 33),
    WALLET_FLAG_DESCRIPTORS = (1ULL << 34),
    WALLET_FLAG_EXTERNAL_SIGNER = (1ULL << 35),
};
fs::path GetWalletDir();
class WalletDescriptor
{
public:
    std::shared_ptr<Descriptor> descriptor;
    uint64_t creation_time = 0;
    int32_t range_start = 0;
    int32_t range_end = 0;
    int32_t next_index = 0;
    DescriptorCache cache;
    void DeserializeDescriptor(const std::string& str)
    {
        std::string error;
        FlatSigningProvider keys;
        descriptor = Parse(str, keys, error, true);
        if (!descriptor) {
            throw std::ios_base::failure("Invalid descriptor: " + error);
        }
    }
    SERIALIZE_METHODS(WalletDescriptor, obj)
    {
        std::string descriptor_str;
        SER_WRITE(obj, descriptor_str = obj.descriptor->ToString());
        READWRITE(descriptor_str, obj.creation_time, obj.next_index, obj.range_start, obj.range_end);
        SER_READ(obj, obj.DeserializeDescriptor(descriptor_str));
    }
    WalletDescriptor() {}
    WalletDescriptor(std::shared_ptr<Descriptor> descriptor, uint64_t creation_time, int32_t range_start, int32_t range_end, int32_t next_index) : descriptor(descriptor), creation_time(creation_time), range_start(range_start), range_end(range_end), next_index(next_index) {}
};
class CWallet;
class DescriptorScriptPubKeyMan;
struct MigrationData
{
    CExtKey master_key;
    std::vector<std::pair<std::string, int64_t>> watch_descs;
    std::vector<std::pair<std::string, int64_t>> solvable_descs;
    std::vector<std::unique_ptr<DescriptorScriptPubKeyMan>> desc_spkms;
    std::shared_ptr<CWallet> watchonly_wallet{nullptr};
    std::shared_ptr<CWallet> solvable_wallet{nullptr};
};
}
#endif
