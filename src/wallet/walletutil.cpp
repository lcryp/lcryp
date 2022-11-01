#include <wallet/walletutil.h>
#include <logging.h>
#include <util/system.h>
namespace wallet {
fs::path GetWalletDir()
{
    fs::path path;
    if (gArgs.IsArgSet("-walletdir")) {
        path = gArgs.GetPathArg("-walletdir");
        if (!fs::is_directory(path)) {
            path = "";
        }
    } else {
        path = gArgs.GetDataDirNet();
        if (fs::is_directory(path / "wallets")) {
            path /= "wallets";
        }
    }
    return path;
}
bool IsFeatureSupported(int wallet_version, int feature_version)
{
    return wallet_version >= feature_version;
}
WalletFeature GetClosestWalletFeature(int version)
{
    static constexpr std::array wallet_features{FEATURE_LATEST, FEATURE_PRE_SPLIT_KEYPOOL, FEATURE_NO_DEFAULT_KEY, FEATURE_HD_SPLIT, FEATURE_HD, FEATURE_COMPRPUBKEY, FEATURE_WALLETCRYPT, FEATURE_BASE};
    for (const WalletFeature& wf : wallet_features) {
        if (version >= wf) return wf;
    }
    return static_cast<WalletFeature>(0);
}
}
