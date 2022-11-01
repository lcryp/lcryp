#ifndef LCRYP_QT_PAYMENTSERVER_H
#define LCRYP_QT_PAYMENTSERVER_H
#if defined(HAVE_CONFIG_H)
#include <config/lcryp-config.h>
#endif
#include <qt/sendcoinsrecipient.h>
#include <QObject>
#include <QString>
class OptionsModel;
namespace interfaces {
class Node;
}
QT_BEGIN_NAMESPACE
class QApplication;
class QByteArray;
class QLocalServer;
class QUrl;
QT_END_NAMESPACE
class PaymentServer : public QObject
{
    Q_OBJECT
public:
    static void ipcParseCommandLine(int argc, char *argv[]);
    static bool ipcSendCommandLine();
    explicit PaymentServer(QObject* parent, bool startLocalServer = true);
    ~PaymentServer();
    void setOptionsModel(OptionsModel *optionsModel);
Q_SIGNALS:
    void receivedPaymentRequest(SendCoinsRecipient);
    void message(const QString &title, const QString &message, unsigned int style);
public Q_SLOTS:
    void uiReady();
    void handleURIOrFile(const QString& s);
private Q_SLOTS:
    void handleURIConnection();
protected:
    bool eventFilter(QObject *object, QEvent *event) override;
private:
    bool saveURIs;
    QLocalServer* uriServer;
    OptionsModel *optionsModel;
};
#endif
