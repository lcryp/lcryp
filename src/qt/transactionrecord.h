#ifndef LCRYP_QT_TRANSACTIONRECORD_H
#define LCRYP_QT_TRANSACTIONRECORD_H
#include <consensus/amount.h>
#include <uint256.h>
#include <QList>
#include <QString>
namespace interfaces {
class Node;
class Wallet;
struct WalletTx;
struct WalletTxStatus;
}
struct TransactionStatus {
    enum Status {
        Confirmed,
        Unconfirmed,
        Confirming,
        Conflicted,
        Abandoned,
        Immature,
        NotAccepted
    };
    bool countsForBalance{false};
    std::string sortKey;
    int matures_in{0};
    Status status{Unconfirmed};
    qint64 depth{0};
    uint256 m_cur_block_hash{};
    bool needsUpdate{false};
};
class TransactionRecord
{
public:
    enum Type
    {
        Other,
        Generated,
        SendToAddress,
        SendToOther,
        RecvWithAddress,
        RecvFromOther,
        SendToSelf
    };
    static const int RecommendedNumConfirmations = 6;
    TransactionRecord():
            hash(), time(0), type(Other), address(""), debit(0), credit(0), idx(0)
    {
    }
    TransactionRecord(uint256 _hash, qint64 _time):
            hash(_hash), time(_time), type(Other), address(""), debit(0),
            credit(0), idx(0)
    {
    }
    TransactionRecord(uint256 _hash, qint64 _time,
                Type _type, const std::string &_address,
                const CAmount& _debit, const CAmount& _credit):
            hash(_hash), time(_time), type(_type), address(_address), debit(_debit), credit(_credit),
            idx(0)
    {
    }
    static bool showTransaction();
    static QList<TransactionRecord> decomposeTransaction(const interfaces::WalletTx& wtx);
    uint256 hash;
    qint64 time;
    Type type;
    std::string address;
    CAmount debit;
    CAmount credit;
    int idx;
    TransactionStatus status;
    bool involvesWatchAddress;
    QString getTxHash() const;
    int getOutputIndex() const;
    void updateStatus(const interfaces::WalletTxStatus& wtx, const uint256& block_hash, int numBlocks, int64_t block_time);
    bool statusUpdateNeeded(const uint256& block_hash) const;
};
#endif
