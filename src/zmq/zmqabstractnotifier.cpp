#include <zmq/zmqabstractnotifier.h>
#include <cassert>
const int CZMQAbstractNotifier::DEFAULT_ZMQ_SNDHWM;
CZMQAbstractNotifier::~CZMQAbstractNotifier()
{
    assert(!psocket);
}
bool CZMQAbstractNotifier::NotifyBlock(const CBlockIndex *  )
{
    return true;
}
bool CZMQAbstractNotifier::NotifyTransaction(const CTransaction & )
{
    return true;
}
bool CZMQAbstractNotifier::NotifyBlockConnect(const CBlockIndex *  )
{
    return true;
}
bool CZMQAbstractNotifier::NotifyBlockDisconnect(const CBlockIndex *  )
{
    return true;
}
bool CZMQAbstractNotifier::NotifyTransactionAcceptance(const CTransaction & , uint64_t mempool_sequence)
{
    return true;
}
bool CZMQAbstractNotifier::NotifyTransactionRemoval(const CTransaction & , uint64_t mempool_sequence)
{
    return true;
}
