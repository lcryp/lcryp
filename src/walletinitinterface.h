#ifndef LCRYP_WALLETINITINTERFACE_H
#define LCRYP_WALLETINITINTERFACE_H
class ArgsManager;
namespace node {
struct NodeContext;
}
class WalletInitInterface {
public:
    virtual bool HasWalletSupport() const = 0;
    virtual void AddWalletOptions(ArgsManager& argsman) const = 0;
    virtual bool ParameterInteraction() const = 0;
    virtual void Construct(node::NodeContext& node) const = 0;
    virtual ~WalletInitInterface() {}
};
extern const WalletInitInterface& g_wallet_init_interface;
#endif
