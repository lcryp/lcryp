#ifndef LCRYP_VALIDATIONINTERFACE_H
#define LCRYP_VALIDATIONINTERFACE_H
#include <primitives/transaction.h>
#include <sync.h>
#include <functional>
#include <memory>
extern RecursiveMutex cs_main;
class BlockValidationState;
class CBlock;
class CBlockIndex;
struct CBlockLocator;
class CValidationInterface;
class CScheduler;
enum class MemPoolRemovalReason;
void RegisterValidationInterface(CValidationInterface* callbacks);
void UnregisterValidationInterface(CValidationInterface* callbacks);
void UnregisterAllValidationInterfaces();
void RegisterSharedValidationInterface(std::shared_ptr<CValidationInterface> callbacks);
void UnregisterSharedValidationInterface(std::shared_ptr<CValidationInterface> callbacks);
void CallFunctionInValidationInterfaceQueue(std::function<void ()> func);
void SyncWithValidationInterfaceQueue() LOCKS_EXCLUDED(cs_main);
class CValidationInterface {
protected:
    ~CValidationInterface() = default;
    virtual void UpdatedBlockTip(const CBlockIndex *pindexNew, const CBlockIndex *pindexFork, bool fInitialDownload) {}
    virtual void TransactionAddedToMempool(const CTransactionRef& tx, uint64_t mempool_sequence) {}
    virtual void TransactionRemovedFromMempool(const CTransactionRef& tx, MemPoolRemovalReason reason, uint64_t mempool_sequence) {}
    virtual void BlockConnected(const std::shared_ptr<const CBlock> &block, const CBlockIndex *pindex) {}
    virtual void BlockDisconnected(const std::shared_ptr<const CBlock> &block, const CBlockIndex* pindex) {}
    virtual void ChainStateFlushed(const CBlockLocator &locator) {}
    virtual void BlockChecked(const CBlock&, const BlockValidationState&) {}
    virtual void NewPoWValidBlock(const CBlockIndex *pindex, const std::shared_ptr<const CBlock>& block) {};
    friend class CMainSignals;
    friend class ValidationInterfaceTest;
};
class MainSignalsImpl;
class CMainSignals {
private:
    std::unique_ptr<MainSignalsImpl> m_internals;
    friend void ::RegisterSharedValidationInterface(std::shared_ptr<CValidationInterface>);
    friend void ::UnregisterValidationInterface(CValidationInterface*);
    friend void ::UnregisterAllValidationInterfaces();
    friend void ::CallFunctionInValidationInterfaceQueue(std::function<void ()> func);
public:
    void RegisterBackgroundSignalScheduler(CScheduler& scheduler);
    void UnregisterBackgroundSignalScheduler();
    void FlushBackgroundCallbacks();
    size_t CallbacksPending();
    void UpdatedBlockTip(const CBlockIndex *, const CBlockIndex *, bool fInitialDownload);
    void TransactionAddedToMempool(const CTransactionRef&, uint64_t mempool_sequence);
    void TransactionRemovedFromMempool(const CTransactionRef&, MemPoolRemovalReason, uint64_t mempool_sequence);
    void BlockConnected(const std::shared_ptr<const CBlock> &, const CBlockIndex *pindex);
    void BlockDisconnected(const std::shared_ptr<const CBlock> &, const CBlockIndex* pindex);
    void ChainStateFlushed(const CBlockLocator &);
    void BlockChecked(const CBlock&, const BlockValidationState&);
    void NewPoWValidBlock(const CBlockIndex *, const std::shared_ptr<const CBlock>&);
};
CMainSignals& GetMainSignals();
#endif
