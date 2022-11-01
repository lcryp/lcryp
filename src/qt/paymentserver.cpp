#if defined(HAVE_CONFIG_H)
#include <config/lcryp-config.h>
#endif
#include <qt/paymentserver.h>
#include <qt/lcrypunits.h>
#include <qt/guiutil.h>
#include <qt/optionsmodel.h>
#include <chainparams.h>
#include <interfaces/node.h>
#include <key_io.h>
#include <node/interface_ui.h>
#include <policy/policy.h>
#include <util/system.h>
#include <wallet/wallet.h>
#include <cstdlib>
#include <memory>
#include <QApplication>
#include <QByteArray>
#include <QDataStream>
#include <QDebug>
#include <QFile>
#include <QFileOpenEvent>
#include <QHash>
#include <QList>
#include <QLocalServer>
#include <QLocalSocket>
#include <QStringList>
#include <QUrlQuery>
const int LCRYP_IPC_CONNECT_TIMEOUT = 1000;
const QString LCRYP_IPC_PREFIX("lcryp:");
static QString ipcServerName()
{
    QString name("LcRypQt");
    QString ddir(GUIUtil::PathToQString(gArgs.GetDataDirNet()));
    name.append(QString::number(qHash(ddir)));
    return name;
}
static QSet<QString> savedPaymentRequests;
void PaymentServer::ipcParseCommandLine(int argc, char* argv[])
{
    for (int i = 1; i < argc; i++)
    {
        QString arg(argv[i]);
        if (arg.startsWith("-")) continue;
        if (arg.startsWith(LCRYP_IPC_PREFIX, Qt::CaseInsensitive))
        {
            savedPaymentRequests.insert(arg);
        }
    }
}
bool PaymentServer::ipcSendCommandLine()
{
    bool fResult = false;
    for (const QString& r : savedPaymentRequests)
    {
        QLocalSocket* socket = new QLocalSocket();
        socket->connectToServer(ipcServerName(), QIODevice::WriteOnly);
        if (!socket->waitForConnected(LCRYP_IPC_CONNECT_TIMEOUT))
        {
            delete socket;
            socket = nullptr;
            return false;
        }
        QByteArray block;
        QDataStream out(&block, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_4_0);
        out << r;
        out.device()->seek(0);
        socket->write(block);
        socket->flush();
        socket->waitForBytesWritten(LCRYP_IPC_CONNECT_TIMEOUT);
        socket->disconnectFromServer();
        delete socket;
        socket = nullptr;
        fResult = true;
    }
    return fResult;
}
PaymentServer::PaymentServer(QObject* parent, bool startLocalServer) :
    QObject(parent),
    saveURIs(true),
    uriServer(nullptr),
    optionsModel(nullptr)
{
    if (parent)
        parent->installEventFilter(this);
    QString name = ipcServerName();
    QLocalServer::removeServer(name);
    if (startLocalServer)
    {
        uriServer = new QLocalServer(this);
        if (!uriServer->listen(name)) {
            QMessageBox::critical(nullptr, tr("Payment request error"),
                tr("Cannot start lcryp: click-to-pay handler"));
        }
        else {
            connect(uriServer, &QLocalServer::newConnection, this, &PaymentServer::handleURIConnection);
        }
    }
}
PaymentServer::~PaymentServer() = default;
bool PaymentServer::eventFilter(QObject *object, QEvent *event)
{
    if (event->type() == QEvent::FileOpen) {
        QFileOpenEvent *fileEvent = static_cast<QFileOpenEvent*>(event);
        if (!fileEvent->file().isEmpty())
            handleURIOrFile(fileEvent->file());
        else if (!fileEvent->url().isEmpty())
            handleURIOrFile(fileEvent->url().toString());
        return true;
    }
    return QObject::eventFilter(object, event);
}
void PaymentServer::uiReady()
{
    saveURIs = false;
    for (const QString& s : savedPaymentRequests)
    {
        handleURIOrFile(s);
    }
    savedPaymentRequests.clear();
}
void PaymentServer::handleURIOrFile(const QString& s)
{
    if (saveURIs)
    {
        savedPaymentRequests.insert(s);
        return;
    }
    if (s.startsWith("lcryp://", Qt::CaseInsensitive))
    {
        Q_EMIT message(tr("URI handling"), tr("'lcryp://' is not a valid URI. Use 'lcryp:' instead."),
            CClientUIInterface::MSG_ERROR);
    }
    else if (s.startsWith(LCRYP_IPC_PREFIX, Qt::CaseInsensitive))
    {
        QUrlQuery uri((QUrl(s)));
        {
            SendCoinsRecipient recipient;
            if (GUIUtil::parseLcRypURI(s, &recipient))
            {
                std::string error_msg;
                const CTxDestination dest = DecodeDestination(recipient.address.toStdString(), error_msg);
                if (!IsValidDestination(dest)) {
                    if (uri.hasQueryItem("r")) {
                        Q_EMIT message(tr("URI handling"),
                            tr("Cannot process payment request because BIP70 is not supported.\n"
                               "Due to widespread security flaws in BIP70 it's strongly recommended that any merchant instructions to switch wallets be ignored.\n"
                               "If you are receiving this error you should request the merchant provide a BIP21 compatible URI."),
                            CClientUIInterface::ICON_WARNING);
                    }
                    Q_EMIT message(tr("URI handling"), QString::fromStdString(error_msg),
                        CClientUIInterface::MSG_ERROR);
                }
                else
                    Q_EMIT receivedPaymentRequest(recipient);
            }
            else
                Q_EMIT message(tr("URI handling"),
                    tr("URI cannot be parsed! This can be caused by an invalid LcRyp address or malformed URI parameters."),
                    CClientUIInterface::ICON_WARNING);
            return;
        }
    }
    if (QFile::exists(s))
    {
        Q_EMIT message(tr("Payment request file handling"),
            tr("Cannot process payment request because BIP70 is not supported.\n"
               "Due to widespread security flaws in BIP70 it's strongly recommended that any merchant instructions to switch wallets be ignored.\n"
               "If you are receiving this error you should request the merchant provide a BIP21 compatible URI."),
            CClientUIInterface::ICON_WARNING);
    }
}
void PaymentServer::handleURIConnection()
{
    QLocalSocket *clientConnection = uriServer->nextPendingConnection();
    while (clientConnection->bytesAvailable() < (int)sizeof(quint32))
        clientConnection->waitForReadyRead();
    connect(clientConnection, &QLocalSocket::disconnected, clientConnection, &QLocalSocket::deleteLater);
    QDataStream in(clientConnection);
    in.setVersion(QDataStream::Qt_4_0);
    if (clientConnection->bytesAvailable() < (int)sizeof(quint16)) {
        return;
    }
    QString msg;
    in >> msg;
    handleURIOrFile(msg);
}
void PaymentServer::setOptionsModel(OptionsModel *_optionsModel)
{
    this->optionsModel = _optionsModel;
}
