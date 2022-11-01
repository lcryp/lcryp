#ifndef LCRYP_QT_WINSHUTDOWNMONITOR_H
#define LCRYP_QT_WINSHUTDOWNMONITOR_H
#ifdef WIN32
#include <QByteArray>
#include <QString>
#include <windef.h>
#include <QAbstractNativeEventFilter>
class WinShutdownMonitor : public QAbstractNativeEventFilter
{
public:
    bool nativeEventFilter(const QByteArray &eventType, void *pMessage, long *pnResult) override;
    static void registerShutdownBlockReason(const QString& strReason, const HWND& mainWinId);
};
#endif
#endif
