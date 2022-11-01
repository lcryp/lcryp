#include <qt/networkstyle.h>
#include <qt/guiconstants.h>
#include <chainparamsbase.h>
#include <tinyformat.h>
#include <QApplication>
static const struct {
    const char *networkId;
    const char *appName;
    const int iconColorHueShift;
    const int iconColorSaturationReduction;
} network_styles[] = {
    {"main", QAPP_APP_NAME_DEFAULT, 0, 0},
    {"test", QAPP_APP_NAME_TESTNET, 70, 30},
    {"signet", QAPP_APP_NAME_SIGNET, 35, 15},
    {"regtest", QAPP_APP_NAME_REGTEST, 160, 30},
};
NetworkStyle::NetworkStyle(const QString &_appName, const int iconColorHueShift, const int iconColorSaturationReduction, const char *_titleAddText):
    appName(_appName),
    titleAddText(qApp->translate("SplashScreen", _titleAddText))
{
    QPixmap pixmap(":/icons/lcryp");
    if(iconColorHueShift != 0 && iconColorSaturationReduction != 0)
    {
        QImage img = pixmap.toImage();
        int h,s,l,a;
        for(int y=0;y<img.height();y++)
        {
            QRgb *scL = reinterpret_cast< QRgb *>( img.scanLine( y ) );
            for(int x=0;x<img.width();x++)
            {
                a = qAlpha(scL[x]);
                QColor col(scL[x]);
                col.getHsl(&h,&s,&l);
                h+=iconColorHueShift;
                if(s>iconColorSaturationReduction)
                {
                    s -= iconColorSaturationReduction;
                }
                col.setHsl(h,s,l,a);
                scL[x] = col.rgba();
            }
        }
        pixmap.convertFromImage(img);
    }
    appIcon             = QIcon(pixmap);
    trayAndWindowIcon   = QIcon(pixmap.scaled(QSize(256,256)));
}
const NetworkStyle* NetworkStyle::instantiate(const std::string& networkId)
{
    std::string titleAddText = networkId == CBaseChainParams::MAIN ? "" : strprintf("[%s]", networkId);
    for (const auto& network_style : network_styles) {
        if (networkId == network_style.networkId) {
            return new NetworkStyle(
                    network_style.appName,
                    network_style.iconColorHueShift,
                    network_style.iconColorSaturationReduction,
                    titleAddText.c_str());
        }
    }
    return nullptr;
}
