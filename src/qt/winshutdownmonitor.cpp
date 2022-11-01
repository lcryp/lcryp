#include <qt/winshutdownmonitor.h>
#if defined(Q_OS_WIN)
#include <shutdown.h>
#include <windows.h>
#include <QDebug>
bool WinShutdownMonitor::nativeEventFilter(const QByteArray &eventType, void *pMessage, long *pnResult)
{
       Q_UNUSED(eventType);
       MSG *pMsg = static_cast<MSG *>(pMessage);
       switch(pMsg->message)
       {
           case WM_QUERYENDSESSION:
           {
               StartShutdown();
               *pnResult = FALSE;
               return true;
           }
           case WM_ENDSESSION:
           {
               *pnResult = FALSE;
               return true;
           }
       }
       return false;
}
void WinShutdownMonitor::registerShutdownBlockReason(const QString& strReason, const HWND& mainWinId)
{
    typedef BOOL (WINAPI *PSHUTDOWNBRCREATE)(HWND, LPCWSTR);
    PSHUTDOWNBRCREATE shutdownBRCreate = (PSHUTDOWNBRCREATE)GetProcAddress(GetModuleHandleA("User32.dll"), "ShutdownBlockReasonCreate");
    if (shutdownBRCreate == nullptr) {
        qWarning() << "registerShutdownBlockReason: GetProcAddress for ShutdownBlockReasonCreate failed";
        return;
    }
    if (shutdownBRCreate(mainWinId, strReason.toStdWString().c_str()))
        qInfo() << "registerShutdownBlockReason: Successfully registered: " + strReason;
    else
        qWarning() << "registerShutdownBlockReason: Failed to register: " + strReason;
}
#endif
