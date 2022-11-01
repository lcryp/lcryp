#ifndef LCRYP_QT_MACDOCKICONHANDLER_H
#define LCRYP_QT_MACDOCKICONHANDLER_H
#include <QObject>
class MacDockIconHandler : public QObject
{
    Q_OBJECT
public:
    static MacDockIconHandler *instance();
    static void cleanup();
Q_SIGNALS:
    void dockIconClicked();
private:
    MacDockIconHandler();
};
#endif
