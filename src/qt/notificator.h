#ifndef LCRYP_QT_NOTIFICATOR_H
#define LCRYP_QT_NOTIFICATOR_H
#if defined(HAVE_CONFIG_H)
#include <config/lcryp-config.h>
#endif
#include <QIcon>
#include <QObject>
QT_BEGIN_NAMESPACE
class QSystemTrayIcon;
#ifdef USE_DBUS
class QDBusInterface;
#endif
QT_END_NAMESPACE
class Notificator: public QObject
{
    Q_OBJECT
public:
    Notificator(const QString &programName, QSystemTrayIcon *trayIcon, QWidget *parent);
    ~Notificator();
    enum Class
    {
        Information,
        Warning,
        Critical
    };
public Q_SLOTS:
    void notify(Class cls, const QString &title, const QString &text,
                const QIcon &icon = QIcon(), int millisTimeout = 10000);
private:
    QWidget *parent;
    enum Mode {
        None,
        Freedesktop,
        QSystemTray,
        UserNotificationCenter
    };
    QString programName;
    Mode mode;
    QSystemTrayIcon *trayIcon;
#ifdef USE_DBUS
    QDBusInterface *interface;
    void notifyDBus(Class cls, const QString &title, const QString &text, const QIcon &icon, int millisTimeout);
#endif
    void notifySystray(Class cls, const QString &title, const QString &text, int millisTimeout);
#ifdef Q_OS_MACOS
    void notifyMacUserNotificationCenter(const QString &title, const QString &text);
#endif
};
#endif
