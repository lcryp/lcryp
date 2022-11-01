#ifndef LCRYP_QT_OPTIONSMODEL_H
#define LCRYP_QT_OPTIONSMODEL_H
#include <cstdint>
#include <qt/lcrypunits.h>
#include <qt/guiconstants.h>
#include <QAbstractListModel>
#include <assert.h>
struct bilingual_str;
namespace interfaces {
class Node;
}
extern const char *DEFAULT_GUI_PROXY_HOST;
static constexpr uint16_t DEFAULT_GUI_PROXY_PORT = 9050;
static inline int PruneMiBtoGB(int64_t mib) { return (mib * 1024 * 1024 + GB_BYTES - 1) / GB_BYTES; }
static inline int64_t PruneGBtoMiB(int gb) { return gb * GB_BYTES / 1024 / 1024; }
class OptionsModel : public QAbstractListModel
{
    Q_OBJECT
public:
    explicit OptionsModel(interfaces::Node& node, QObject *parent = nullptr);
    enum OptionID {
        StartAtStartup,
        ShowTrayIcon,
        MinimizeToTray,
        MapPortUPnP,
        MapPortNatpmp,
        MinimizeOnClose,
        ProxyUse,
        ProxyIP,
        ProxyPort,
        ProxyUseTor,
        ProxyIPTor,
        ProxyPortTor,
        DisplayUnit,
        ThirdPartyTxUrls,
        Language,
        UseEmbeddedMonospacedFont,
        CoinControlFeatures,
        SubFeeFromAmount,
        ThreadsScriptVerif,
        Prune,
        PruneSize,
        DatabaseCache,
        ExternalSignerPath,
        SpendZeroConfChange,
        Listen,
        Server,
        EnablePSBTControls,
        OptionIDRowCount,
    };
    bool Init(bilingual_str& error);
    void Reset();
    int rowCount(const QModelIndex & parent = QModelIndex()) const override;
    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex & index, const QVariant & value, int role = Qt::EditRole) override;
    QVariant getOption(OptionID option) const;
    bool setOption(OptionID option, const QVariant& value);
    void setDisplayUnit(const QVariant& new_unit);
    bool getShowTrayIcon() const { return m_show_tray_icon; }
    bool getMinimizeToTray() const { return fMinimizeToTray; }
    bool getMinimizeOnClose() const { return fMinimizeOnClose; }
    LcRypUnit getDisplayUnit() const { return m_display_lcryp_unit; }
    QString getThirdPartyTxUrls() const { return strThirdPartyTxUrls; }
    bool getUseEmbeddedMonospacedFont() const { return m_use_embedded_monospaced_font; }
    bool getCoinControlFeatures() const { return fCoinControlFeatures; }
    bool getSubFeeFromAmount() const { return m_sub_fee_from_amount; }
    bool getEnablePSBTControls() const { return m_enable_psbt_controls; }
    const QString& getOverriddenByCommandLine() { return strOverriddenByCommandLine; }
    void SetPruneTargetGB(int prune_target_gb);
    void setRestartRequired(bool fRequired);
    bool isRestartRequired() const;
    interfaces::Node& node() const { return m_node; }
private:
    interfaces::Node& m_node;
    bool m_show_tray_icon;
    bool fMinimizeToTray;
    bool fMinimizeOnClose;
    QString language;
    LcRypUnit m_display_lcryp_unit;
    QString strThirdPartyTxUrls;
    bool m_use_embedded_monospaced_font;
    bool fCoinControlFeatures;
    bool m_sub_fee_from_amount;
    bool m_enable_psbt_controls;
    int m_prune_size_gb;
    QString m_proxy_ip;
    QString m_proxy_port;
    QString m_onion_ip;
    QString m_onion_port;
    QString strOverriddenByCommandLine;
    void addOverriddenOption(const std::string &option);
    void checkAndMigrate();
Q_SIGNALS:
    void displayUnitChanged(LcRypUnit unit);
    void coinControlFeaturesChanged(bool);
    void showTrayIconChanged(bool);
    void useEmbeddedMonospacedFontChanged(bool);
};
#endif
