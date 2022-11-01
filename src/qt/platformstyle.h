#ifndef LCRYP_QT_PLATFORMSTYLE_H
#define LCRYP_QT_PLATFORMSTYLE_H
#include <QIcon>
#include <QPixmap>
#include <QString>
class PlatformStyle
{
public:
    static const PlatformStyle *instantiate(const QString &platformId);
    const QString &getName() const { return name; }
    bool getImagesOnButtons() const { return imagesOnButtons; }
    bool getUseExtraSpacing() const { return useExtraSpacing; }
    QColor TextColor() const;
    QColor SingleColor() const;
    QImage SingleColorImage(const QString& filename) const;
    QIcon SingleColorIcon(const QString& filename) const;
    QIcon SingleColorIcon(const QIcon& icon) const;
    QIcon TextColorIcon(const QIcon& icon) const;
private:
    PlatformStyle(const QString &name, bool imagesOnButtons, bool colorizeIcons, bool useExtraSpacing);
    QString name;
    bool imagesOnButtons;
    bool colorizeIcons;
    bool useExtraSpacing;
};
#endif
