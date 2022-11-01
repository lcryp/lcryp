#ifndef LCRYP_QT_LCRYP_H
#define LCRYP_QT_LCRYP_H
#if defined(HAVE_CONFIG_H)
#include <config/lcryp-config.h>
#endif
#include <interfaces/node.h>
#include <qt/initexecutor.h>
#include <assert.h>
#include <memory>
#include <optional>
#include <QApplication>
class LcRypGUI;
class ClientModel;
class NetworkStyle;
class OptionsModel;
class PaymentServer;
class PlatformStyle;
class SplashScreen;
class WalletController;
class WalletModel;
namespace interfaces {
class Init;
}
class LcRypApplication: public QApplication
{
    Q_OBJECT
public:
    explicit LcRypApplication();
    ~LcRypApplication();
#ifdef ENABLE_WALLET
    void createPaymentServer();
#endif
    void parameterSetup();
    [[nodiscard]] bool createOptionsModel(bool resetSettings);
    void InitPruneSetting(int64_t prune_MiB);
    void createWindow(const NetworkStyle *networkStyle);
    void createSplashScreen(const NetworkStyle *networkStyle);
    void createNode(interfaces::Init& init);
    bool baseInitialize();
    void requestInitialize();
    int getReturnValue() const { return returnValue; }
    WId getMainWinId() const;
    void setupPlatformStyle();
    interfaces::Node& node() const { assert(m_node); return *m_node; }
public Q_SLOTS:
    void initializeResult(bool success, interfaces::BlockAndHeaderTipInfo tip_info);
    void requestShutdown();
    void handleRunawayException(const QString &message);
    void handleNonFatalException(const QString& message);
Q_SIGNALS:
    void requestedInitialize();
    void requestedShutdown();
    void splashFinished();
    void windowShown(LcRypGUI* window);
protected:
    bool event(QEvent* e) override;
private:
    std::optional<InitExecutor> m_executor;
    OptionsModel *optionsModel;
    ClientModel *clientModel;
    LcRypGUI *window;
    QTimer *pollShutdownTimer;
#ifdef ENABLE_WALLET
    PaymentServer* paymentServer{nullptr};
    WalletController* m_wallet_controller{nullptr};
#endif
    int returnValue;
    const PlatformStyle *platformStyle;
    std::unique_ptr<QWidget> shutdownWindow;
    SplashScreen* m_splash = nullptr;
    std::unique_ptr<interfaces::Node> m_node;
    void startThread();
};
int GuiMain(int argc, char* argv[]);
#endif
