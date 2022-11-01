#ifndef LCRYP_ZMQ_ZMQABSTRACTNOTIFIER_H
#define LCRYP_ZMQ_ZMQABSTRACTNOTIFIER_H
#include <memory>
#include <string>
class CBlockIndex;
class CTransaction;
class CZMQAbstractNotifier;
using CZMQNotifierFactory = std::unique_ptr<CZMQAbstractNotifier> (*)();
class CZMQAbstractNotifier
{
public:
    static const int DEFAULT_ZMQ_SNDHWM {1000};
    CZMQAbstractNotifier() : psocket(nullptr), outbound_message_high_water_mark(DEFAULT_ZMQ_SNDHWM) { }
    virtual ~CZMQAbstractNotifier();
    template <typename T>
    static std::unique_ptr<CZMQAbstractNotifier> Create()
    {
        return std::make_unique<T>();
    }
    std::string GetType() const { return type; }
    void SetType(const std::string &t) { type = t; }
    std::string GetAddress() const { return address; }
    void SetAddress(const std::string &a) { address = a; }
    int GetOutboundMessageHighWaterMark() const { return outbound_message_high_water_mark; }
    void SetOutboundMessageHighWaterMark(const int sndhwm) {
        if (sndhwm >= 0) {
            outbound_message_high_water_mark = sndhwm;
        }
    }
    virtual bool Initialize(void *pcontext) = 0;
    virtual void Shutdown() = 0;
    virtual bool NotifyBlock(const CBlockIndex *pindex);
    virtual bool NotifyBlockConnect(const CBlockIndex *pindex);
    virtual bool NotifyBlockDisconnect(const CBlockIndex *pindex);
    virtual bool NotifyTransactionAcceptance(const CTransaction &transaction, uint64_t mempool_sequence);
    virtual bool NotifyTransactionRemoval(const CTransaction &transaction, uint64_t mempool_sequence);
    virtual bool NotifyTransaction(const CTransaction &transaction);
protected:
    void *psocket;
    std::string type;
    std::string address;
    int outbound_message_high_water_mark;
};
#endif
