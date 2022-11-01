#include <zmq/zmqnotificationinterface.h>
#include <zmq/zmqpublishnotifier.h>
#include <zmq/zmqutil.h>
#include <zmq.h>
#include <primitives/block.h>
#include <util/system.h>
CZMQNotificationInterface::CZMQNotificationInterface() : pcontext(nullptr)
{
}
CZMQNotificationInterface::~CZMQNotificationInterface()
{
    Shutdown();
}
std::list<const CZMQAbstractNotifier*> CZMQNotificationInterface::GetActiveNotifiers() const
{
    std::list<const CZMQAbstractNotifier*> result;
    for (const auto& n : notifiers) {
        result.push_back(n.get());
    }
    return result;
}
CZMQNotificationInterface* CZMQNotificationInterface::Create()
{
    std::map<std::string, CZMQNotifierFactory> factories;
    factories["pubhashblock"] = CZMQAbstractNotifier::Create<CZMQPublishHashBlockNotifier>;
    factories["pubhashtx"] = CZMQAbstractNotifier::Create<CZMQPublishHashTransactionNotifier>;
    factories["pubrawblock"] = CZMQAbstractNotifier::Create<CZMQPublishRawBlockNotifier>;
    factories["pubrawtx"] = CZMQAbstractNotifier::Create<CZMQPublishRawTransactionNotifier>;
    factories["pubsequence"] = CZMQAbstractNotifier::Create<CZMQPublishSequenceNotifier>;
    std::list<std::unique_ptr<CZMQAbstractNotifier>> notifiers;
    for (const auto& entry : factories)
    {
        std::string arg("-zmq" + entry.first);
        const auto& factory = entry.second;
        for (const std::string& address : gArgs.GetArgs(arg)) {
            std::unique_ptr<CZMQAbstractNotifier> notifier = factory();
            notifier->SetType(entry.first);
            notifier->SetAddress(address);
            notifier->SetOutboundMessageHighWaterMark(static_cast<int>(gArgs.GetIntArg(arg + "hwm", CZMQAbstractNotifier::DEFAULT_ZMQ_SNDHWM)));
            notifiers.push_back(std::move(notifier));
        }
    }
    if (!notifiers.empty())
    {
        std::unique_ptr<CZMQNotificationInterface> notificationInterface(new CZMQNotificationInterface());
        notificationInterface->notifiers = std::move(notifiers);
        if (notificationInterface->Initialize()) {
            return notificationInterface.release();
        }
    }
    return nullptr;
}
bool CZMQNotificationInterface::Initialize()
{
    int major = 0, minor = 0, patch = 0;
    zmq_version(&major, &minor, &patch);
    LogPrint(BCLog::ZMQ, "version %d.%d.%d\n", major, minor, patch);
    LogPrint(BCLog::ZMQ, "Initialize notification interface\n");
    assert(!pcontext);
    pcontext = zmq_ctx_new();
    if (!pcontext)
    {
        zmqError("Unable to initialize context");
        return false;
    }
    for (auto& notifier : notifiers) {
        if (notifier->Initialize(pcontext)) {
            LogPrint(BCLog::ZMQ, "Notifier %s ready (address = %s)\n", notifier->GetType(), notifier->GetAddress());
        } else {
            LogPrint(BCLog::ZMQ, "Notifier %s failed (address = %s)\n", notifier->GetType(), notifier->GetAddress());
            return false;
        }
    }
    return true;
}
void CZMQNotificationInterface::Shutdown()
{
    LogPrint(BCLog::ZMQ, "Shutdown notification interface\n");
    if (pcontext)
    {
        for (auto& notifier : notifiers) {
            LogPrint(BCLog::ZMQ, "Shutdown notifier %s at %s\n", notifier->GetType(), notifier->GetAddress());
            notifier->Shutdown();
        }
        zmq_ctx_term(pcontext);
        pcontext = nullptr;
    }
}
namespace {
template <typename Function>
void TryForEachAndRemoveFailed(std::list<std::unique_ptr<CZMQAbstractNotifier>>& notifiers, const Function& func)
{
    for (auto i = notifiers.begin(); i != notifiers.end(); ) {
        CZMQAbstractNotifier* notifier = i->get();
        if (func(notifier)) {
            ++i;
        } else {
            notifier->Shutdown();
            i = notifiers.erase(i);
        }
    }
}
}
void CZMQNotificationInterface::UpdatedBlockTip(const CBlockIndex *pindexNew, const CBlockIndex *pindexFork, bool fInitialDownload)
{
    if (fInitialDownload || pindexNew == pindexFork)
        return;
    TryForEachAndRemoveFailed(notifiers, [pindexNew](CZMQAbstractNotifier* notifier) {
        return notifier->NotifyBlock(pindexNew);
    });
}
void CZMQNotificationInterface::TransactionAddedToMempool(const CTransactionRef& ptx, uint64_t mempool_sequence)
{
    const CTransaction& tx = *ptx;
    TryForEachAndRemoveFailed(notifiers, [&tx, mempool_sequence](CZMQAbstractNotifier* notifier) {
        return notifier->NotifyTransaction(tx) && notifier->NotifyTransactionAcceptance(tx, mempool_sequence);
    });
}
void CZMQNotificationInterface::TransactionRemovedFromMempool(const CTransactionRef& ptx, MemPoolRemovalReason reason, uint64_t mempool_sequence)
{
    const CTransaction& tx = *ptx;
    TryForEachAndRemoveFailed(notifiers, [&tx, mempool_sequence](CZMQAbstractNotifier* notifier) {
        return notifier->NotifyTransactionRemoval(tx, mempool_sequence);
    });
}
void CZMQNotificationInterface::BlockConnected(const std::shared_ptr<const CBlock>& pblock, const CBlockIndex* pindexConnected)
{
    for (const CTransactionRef& ptx : pblock->vtx) {
        const CTransaction& tx = *ptx;
        TryForEachAndRemoveFailed(notifiers, [&tx](CZMQAbstractNotifier* notifier) {
            return notifier->NotifyTransaction(tx);
        });
    }
    TryForEachAndRemoveFailed(notifiers, [pindexConnected](CZMQAbstractNotifier* notifier) {
        return notifier->NotifyBlockConnect(pindexConnected);
    });
}
void CZMQNotificationInterface::BlockDisconnected(const std::shared_ptr<const CBlock>& pblock, const CBlockIndex* pindexDisconnected)
{
    for (const CTransactionRef& ptx : pblock->vtx) {
        const CTransaction& tx = *ptx;
        TryForEachAndRemoveFailed(notifiers, [&tx](CZMQAbstractNotifier* notifier) {
            return notifier->NotifyTransaction(tx);
        });
    }
    TryForEachAndRemoveFailed(notifiers, [pindexDisconnected](CZMQAbstractNotifier* notifier) {
        return notifier->NotifyBlockDisconnect(pindexDisconnected);
    });
}
CZMQNotificationInterface* g_zmq_notification_interface = nullptr;