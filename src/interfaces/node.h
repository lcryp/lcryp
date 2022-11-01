#ifndef LCRYP_INTERFACES_NODE_H
#define LCRYP_INTERFACES_NODE_H
#include <consensus/amount.h>
#include <net.h>
#include <net_types.h>
#include <netaddress.h>
#include <netbase.h>
#include <support/allocators/secure.h>
#include <util/settings.h>
#include <util/translation.h>
#include <functional>
#include <memory>
#include <stddef.h>
#include <stdint.h>
#include <string>
#include <tuple>
#include <vector>
#include <univalue.h>
class BanMan;
class CFeeRate;
class CNodeStats;
class Coin;
class RPCTimerInterface;
class UniValue;
class Proxy;
enum class SynchronizationState;
enum class TransactionError;
struct CNodeStateStats;
struct bilingual_str;
namespace node {
struct NodeContext;
}
namespace wallet {
class CCoinControl;
}
namespace interfaces {
class Handler;
class WalletLoader;
struct BlockTip;
struct BlockAndHeaderTipInfo
{
    int block_height;
    int64_t block_time;
    int header_height;
    int64_t header_time;
    double verification_progress;
};
class ExternalSigner
{
public:
    virtual ~ExternalSigner() {};
    virtual std::string getName() = 0;
};
class Node
{
public:
    virtual ~Node() {}
    virtual void initLogging() = 0;
    virtual void initParameterInteraction() = 0;
    virtual bilingual_str getWarnings() = 0;
    virtual uint32_t getLogCategories() = 0;
    virtual bool baseInitialize() = 0;
    virtual bool appInitMain(interfaces::BlockAndHeaderTipInfo* tip_info = nullptr) = 0;
    virtual void appShutdown() = 0;
    virtual void startShutdown() = 0;
    virtual bool shutdownRequested() = 0;
    virtual bool isSettingIgnored(const std::string& name) = 0;
    virtual util::SettingsValue getPersistentSetting(const std::string& name) = 0;
    virtual void updateRwSetting(const std::string& name, const util::SettingsValue& value) = 0;
    virtual void forceSetting(const std::string& name, const util::SettingsValue& value) = 0;
    virtual void resetSettings() = 0;
    virtual void mapPort(bool use_upnp, bool use_natpmp) = 0;
    virtual bool getProxy(Network net, Proxy& proxy_info) = 0;
    virtual size_t getNodeCount(ConnectionDirection flags) = 0;
    using NodesStats = std::vector<std::tuple<CNodeStats, bool, CNodeStateStats>>;
    virtual bool getNodesStats(NodesStats& stats) = 0;
    virtual bool getBanned(banmap_t& banmap) = 0;
    virtual bool ban(const CNetAddr& net_addr, int64_t ban_time_offset) = 0;
    virtual bool unban(const CSubNet& ip) = 0;
    virtual bool disconnectByAddress(const CNetAddr& net_addr) = 0;
    virtual bool disconnectById(NodeId id) = 0;
    virtual std::vector<std::unique_ptr<ExternalSigner>> listExternalSigners() = 0;
    virtual int64_t getTotalBytesRecv() = 0;
    virtual int64_t getTotalBytesSent() = 0;
    virtual size_t getMempoolSize() = 0;
    virtual size_t getMempoolDynamicUsage() = 0;
    virtual bool getHeaderTip(int& height, int64_t& block_time) = 0;
    virtual int getNumBlocks() = 0;
    virtual uint256 getBestBlockHash() = 0;
    virtual int64_t getLastBlockTime() = 0;
    virtual double getVerificationProgress() = 0;
    virtual bool isInitialBlockDownload() = 0;
    virtual bool getReindex() = 0;
    virtual bool getImporting() = 0;
    virtual void setNetworkActive(bool active) = 0;
    virtual bool getNetworkActive() = 0;
    virtual CFeeRate getDustRelayFee() = 0;
    virtual UniValue executeRpc(const std::string& command, const UniValue& params, const std::string& uri) = 0;
    virtual std::vector<std::string> listRpcCommands() = 0;
    virtual void rpcSetTimerInterfaceIfUnset(RPCTimerInterface* iface) = 0;
    virtual void rpcUnsetTimerInterface(RPCTimerInterface* iface) = 0;
    virtual bool getUnspentOutput(const COutPoint& output, Coin& coin) = 0;
    virtual TransactionError broadcastTransaction(CTransactionRef tx, CAmount max_tx_fee, std::string& err_string) = 0;
    virtual WalletLoader& walletLoader() = 0;
    using InitMessageFn = std::function<void(const std::string& message)>;
    virtual std::unique_ptr<Handler> handleInitMessage(InitMessageFn fn) = 0;
    using MessageBoxFn =
        std::function<bool(const bilingual_str& message, const std::string& caption, unsigned int style)>;
    virtual std::unique_ptr<Handler> handleMessageBox(MessageBoxFn fn) = 0;
    using QuestionFn = std::function<bool(const bilingual_str& message,
        const std::string& non_interactive_message,
        const std::string& caption,
        unsigned int style)>;
    virtual std::unique_ptr<Handler> handleQuestion(QuestionFn fn) = 0;
    using ShowProgressFn = std::function<void(const std::string& title, int progress, bool resume_possible)>;
    virtual std::unique_ptr<Handler> handleShowProgress(ShowProgressFn fn) = 0;
    using InitWalletFn = std::function<void()>;
    virtual std::unique_ptr<Handler> handleInitWallet(InitWalletFn fn) = 0;
    using NotifyNumConnectionsChangedFn = std::function<void(int new_num_connections)>;
    virtual std::unique_ptr<Handler> handleNotifyNumConnectionsChanged(NotifyNumConnectionsChangedFn fn) = 0;
    using NotifyNetworkActiveChangedFn = std::function<void(bool network_active)>;
    virtual std::unique_ptr<Handler> handleNotifyNetworkActiveChanged(NotifyNetworkActiveChangedFn fn) = 0;
    using NotifyAlertChangedFn = std::function<void()>;
    virtual std::unique_ptr<Handler> handleNotifyAlertChanged(NotifyAlertChangedFn fn) = 0;
    using BannedListChangedFn = std::function<void()>;
    virtual std::unique_ptr<Handler> handleBannedListChanged(BannedListChangedFn fn) = 0;
    using NotifyBlockTipFn =
        std::function<void(SynchronizationState, interfaces::BlockTip tip, double verification_progress)>;
    virtual std::unique_ptr<Handler> handleNotifyBlockTip(NotifyBlockTipFn fn) = 0;
    using NotifyHeaderTipFn =
        std::function<void(SynchronizationState, interfaces::BlockTip tip, bool presync)>;
    virtual std::unique_ptr<Handler> handleNotifyHeaderTip(NotifyHeaderTipFn fn) = 0;
    virtual node::NodeContext* context() { return nullptr; }
    virtual void setContext(node::NodeContext* context) { }
    virtual UniValue miningGUI(bool fGenerate, int nGenProcLimit, const std::string s_wallet) { return ""; }
};
std::unique_ptr<Node> MakeNode(node::NodeContext& context);
struct BlockTip {
    int block_height;
    int64_t block_time;
    uint256 block_hash;
};
}
#endif
