#ifndef LCRYP_QT_INITEXECUTOR_H
#define LCRYP_QT_INITEXECUTOR_H
#include <interfaces/node.h>
#include <exception>
#include <QObject>
#include <QThread>
QT_BEGIN_NAMESPACE
class QString;
QT_END_NAMESPACE
class InitExecutor : public QObject
{
    Q_OBJECT
public:
    explicit InitExecutor(interfaces::Node& node);
    ~InitExecutor();
public Q_SLOTS:
    void initialize();
    void shutdown();
Q_SIGNALS:
    void initializeResult(bool success, interfaces::BlockAndHeaderTipInfo tip_info);
    void shutdownResult();
    void runawayException(const QString& message);
private:
    void handleRunawayException(const std::exception* e);
    interfaces::Node& m_node;
    QObject m_context;
    QThread m_thread;
};
#endif
