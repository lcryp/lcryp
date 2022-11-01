#ifndef LCRYP_QT_NETWORKSTYLE_H
#define LCRYP_QT_NETWORKSTYLE_H
#include <QIcon>
#include <QPixmap>
#include <QString>
class NetworkStyle
{
public:
    static const NetworkStyle* instantiate(const std::string& networkId);
    const QString &getAppName() const { return appName; }
    const QIcon &getAppIcon() const { return appIcon; }
    const QIcon &getTrayAndWindowIcon() const { return trayAndWindowIcon; }
    const QString &getTitleAddText() const { return titleAddText; }
private:
    NetworkStyle(const QString &appName, const int iconColorHueShift, const int iconColorSaturationReduction, const char *titleAddText);
    QString appName;
    QIcon appIcon;
    QIcon trayAndWindowIcon;
    QString titleAddText;
};
#endif
