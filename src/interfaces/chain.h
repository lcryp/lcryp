#ifndef LCRYP_INTERFACES_CHAIN_H
#define LCRYP_INTERFACES_CHAIN_H
#include <primitives/transaction.h>
#include <util/settings.h>
#include <functional>
#include <memory>
#include <optional>
#include <stddef.h>
#include <stdint.h>
#include <string>
#include <vector>
class ArgsManager;
class CBlock;
class CBlockUndo;
class CFeeRate;
class CRPCCommand;
class CScheduler;
class Coin;
class uint256;
enum class MemPoolRemovalReason;
enum class RBFTransactionState;
struct bilingual_str;
struct CBlockLocator;
struct FeeCalculation;
namespace node {
struct NodeContext;
}
namespace interfaces {
class Handler;
class Wallet;
struct BlockKey {
    uint256 hash;
    int height = -1;
};
class FoundBlock
{
public:
    FoundBlock& hash(uint256& hash) { m_hash = &hash; return *this; }
    FoundBlock& height(int& height) { m_height = &height; return *this; }
    FoundBlock& time(int64_t& time) { m_time = &time; return *this; }
    FoundBlock& maxTime(int64_t& max_time) { m_max_time = &max_time; return *this; }
    FoundBlock& mtpTime(int64_t& mtp_time) { m_mtp_time = &mtp_time; return *this; }
    FoundBlock& inActiveChain(bool& in_active_chain) { m_in_active_chain = &in_active_chain; return *this; }
    FoundBlock& locator(CBlockLocator& locator) { m_locator = &locator; return *this; }
    FoundBlock& nextBlock(const FoundBlock& next_block) { m_next_block = &next_block; return *this; }
    FoundBlock& data(CBlock& data) { m_data = &data; return *this; }
    uint256* m_hash = nullptr;
    int* m_height = nullptr;
    int64_t* m_time = nullptr;
    int64_t* m_max_time = nullptr;
    int64_t* m_mtp_time = nullptr;
    bool* m_in_active_chain = nullptr;
    CBlockLocator* m_locator = nullptr;
    const FoundBlock* m_next_block = nullptr;
    CBlock* m_data = nullptr;
    mutable bool found = false;
};
struct BlockInfo {
    const uint256& hash;
    const uint256* prev_hash = nullptr;
    int height = -1;
    int file_number = -1;
    unsigned data_pos = 0;
    const CBlock* data = nullptr;
    const CBlockUndo* undo_data = nullptr;
    BlockInfo(const uint256& hash LIFETIMEBOUND) : hash(hash) {}
};
class Chain
{
public:
    virtual ~Chain() {}
    virtual std::optional<int> getHeight() = 0;
    virtual uint256 getBlockHash(int height) = 0;
    virtual bool haveBlockOnDisk(int height) = 0;
    virtual CBlockLocator getTipLocator() = 0;
    virtual CBlockLocator getActiveChainLocator(const uint256& block_hash) = 0;
    virtual std::optional<int> findLocatorFork(const CBlockLocator& locator) = 0;
    virtual bool findBlock(const uint256& hash, const FoundBlock& block={}) = 0;
    virtual bool findFirstBlockWithTimeAndHeight(int64_t min_time, int min_height, const FoundBlock& block={}) = 0;
    virtual bool findAncestorByHeight(const uint256& block_hash, int ancestor_height, const FoundBlock& ancestor_out={}) = 0;
    virtual bool findAncestorByHash(const uint256& block_hash,
        const uint256& ancestor_hash,
        const FoundBlock& ancestor_out={}) = 0;
    virtual bool findCommonAncestor(const uint256& block_hash1,
        const uint256& block_hash2,
        const FoundBlock& ancestor_out={},
        const FoundBlock& block1_out={},
        const FoundBlock& block2_out={}) = 0;
    virtual void findCoins(std::map<COutPoint, Coin>& coins) = 0;
    virtual double guessVerificationProgress(const uint256& block_hash) = 0;
    virtual bool hasBlocks(const uint256& block_hash, int min_height = 0, std::optional<int> max_height = {}) = 0;
    virtual RBFTransactionState isRBFOptIn(const CTransaction& tx) = 0;
    virtual bool isInMempool(const uint256& txid) = 0;
    virtual bool hasDescendantsInMempool(const uint256& txid) = 0;
    virtual bool broadcastTransaction(const CTransactionRef& tx,
        const CAmount& max_tx_fee,
        bool relay,
        std::string& err_string) = 0;
    virtual void getTransactionAncestry(const uint256& txid, size_t& ancestors, size_t& descendants, size_t* ancestorsize = nullptr, CAmount* ancestorfees = nullptr) = 0;
    virtual void getPackageLimits(unsigned int& limit_ancestor_count, unsigned int& limit_descendant_count) = 0;
    virtual bool checkChainLimits(const CTransactionRef& tx) = 0;
    virtual CFeeRate estimateSmartFee(int num_blocks, bool conservative, FeeCalculation* calc = nullptr) = 0;
    virtual unsigned int estimateMaxBlocks() = 0;
    virtual CFeeRate mempoolMinFee() = 0;
    virtual CFeeRate relayMinFee() = 0;
    virtual CFeeRate relayIncrementalFee() = 0;
    virtual CFeeRate relayDustFee() = 0;
    virtual bool havePruned() = 0;
    virtual bool isReadyToBroadcast() = 0;
    virtual bool isInitialBlockDownload() = 0;
    virtual bool shutdownRequested() = 0;
    virtual void initMessage(const std::string& message) = 0;
    virtual void initWarning(const bilingual_str& message) = 0;
    virtual void initError(const bilingual_str& message) = 0;
    virtual void showProgress(const std::string& title, int progress, bool resume_possible) = 0;
    class Notifications
    {
    public:
        virtual ~Notifications() {}
        virtual void transactionAddedToMempool(const CTransactionRef& tx, uint64_t mempool_sequence) {}
        virtual void transactionRemovedFromMempool(const CTransactionRef& tx, MemPoolRemovalReason reason, uint64_t mempool_sequence) {}
        virtual void blockConnected(const BlockInfo& block) {}
        virtual void blockDisconnected(const BlockInfo& block) {}
        virtual void updatedBlockTip() {}
        virtual void chainStateFlushed(const CBlockLocator& locator) {}
    };
    virtual std::unique_ptr<Handler> handleNotifications(std::shared_ptr<Notifications> notifications) = 0;
    virtual void waitForNotificationsIfTipChanged(const uint256& old_tip) = 0;
    virtual std::unique_ptr<Handler> handleRpc(const CRPCCommand& command) = 0;
    virtual bool rpcEnableDeprecated(const std::string& method) = 0;
    virtual void rpcRunLater(const std::string& name, std::function<void()> fn, int64_t seconds) = 0;
    virtual int rpcSerializationFlags() = 0;
    virtual util::SettingsValue getSetting(const std::string& arg) = 0;
    virtual std::vector<util::SettingsValue> getSettingsList(const std::string& arg) = 0;
    virtual util::SettingsValue getRwSetting(const std::string& name) = 0;
    virtual bool updateRwSetting(const std::string& name, const util::SettingsValue& value, bool write=true) = 0;
    virtual void requestMempoolTransactions(Notifications& notifications) = 0;
    virtual bool hasAssumedValidChain() = 0;
    virtual node::NodeContext* context() { return nullptr; }
};
class ChainClient
{
public:
    virtual ~ChainClient() {}
    virtual void registerRpcs() = 0;
    virtual bool verify() = 0;
    virtual bool load() = 0;
    virtual void start(CScheduler& scheduler) = 0;
    virtual void flush() = 0;
    virtual void stop() = 0;
    virtual void setMockTime(int64_t time) = 0;
};
std::unique_ptr<Chain> MakeChain(node::NodeContext& node);
}
#endif
