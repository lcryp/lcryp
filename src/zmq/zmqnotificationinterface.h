#ifndef LCRYP_ZMQ_ZMQNOTIFICATIONINTERFACE_H
#define LCRYP_ZMQ_ZMQNOTIFICATIONINTERFACE_H
#include <validationinterface.h>
#include <list>
#include <memory>
class CBlockIndex;
class CZMQAbstractNotifier;
class CZMQNotificationInterface final : public CValidationInterface
{
public:
    virtual ~CZMQNotificationInterface();
    std::list<const CZMQAbstractNotifier*> GetActiveNotifiers() const;
    static CZMQNotificationInterface* Create();
protected:
    bool Initialize();
    void Shutdown();
    void TransactionAddedToMempool(const CTransactionRef& tx, uint64_t mempool_sequence) override;
    void TransactionRemovedFromMempool(const CTransactionRef& tx, MemPoolRemovalReason reason, uint64_t mempool_sequence) override;
    void BlockConnected(const std::shared_ptr<const CBlock>& pblock, const CBlockIndex* pindexConnected) override;
    void BlockDisconnected(const std::shared_ptr<const CBlock>& pblock, const CBlockIndex* pindexDisconnected) override;
    void UpdatedBlockTip(const CBlockIndex *pindexNew, const CBlockIndex *pindexFork, bool fInitialDownload) override;
private:
    CZMQNotificationInterface();
    void *pcontext;
    std::list<std::unique_ptr<CZMQAbstractNotifier>> notifiers;
};
extern CZMQNotificationInterface* g_zmq_notification_interface;
#endif
