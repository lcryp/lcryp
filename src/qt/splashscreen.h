#ifndef LCRYP_QT_SPLASHSCREEN_H
#define LCRYP_QT_SPLASHSCREEN_H
#include <QWidget>
#include <memory>
class NetworkStyle;
namespace interfaces {
class Handler;
class Node;
class Wallet;
};
class SplashScreen : public QWidget
{
    Q_OBJECT
public:
    explicit SplashScreen(const NetworkStyle *networkStyle);
    ~SplashScreen();
    void setNode(interfaces::Node& node);
protected:
    void paintEvent(QPaintEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
public Q_SLOTS:
    void finish();
    void showMessage(const QString &message, int alignment, const QColor &color);
    void handleLoadWallet();
protected:
    bool eventFilter(QObject * obj, QEvent * ev) override;
private:
    void subscribeToCoreSignals();
    void unsubscribeFromCoreSignals();
    void shutdown();
    QPixmap pixmap;
    QString curMessage;
    QColor curColor;
    int curAlignment;
    interfaces::Node* m_node = nullptr;
    bool m_shutdown = false;
    std::unique_ptr<interfaces::Handler> m_handler_init_message;
    std::unique_ptr<interfaces::Handler> m_handler_show_progress;
    std::unique_ptr<interfaces::Handler> m_handler_init_wallet;
    std::unique_ptr<interfaces::Handler> m_handler_load_wallet;
    std::list<std::unique_ptr<interfaces::Wallet>> m_connected_wallets;
    std::list<std::unique_ptr<interfaces::Handler>> m_connected_wallet_handlers;
};
#endif
