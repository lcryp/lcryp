#if defined(HAVE_CONFIG_H)
#include <config/lcryp-config.h>
#endif
#include <qt/optionsmodel.h>
#include <qt/lcrypunits.h>
#include <qt/guiconstants.h>
#include <qt/guiutil.h>
#include <interfaces/node.h>
#include <mapport.h>
#include <net.h>
#include <netbase.h>
#include <txdb.h>
#include <util/string.h>
#include <validation.h>
#include <wallet/wallet.h>
#include <QDebug>
#include <QLatin1Char>
#include <QSettings>
#include <QStringList>
#include <QVariant>
#include <univalue.h>
const char *DEFAULT_GUI_PROXY_HOST = "127.0.0.1";
static const QString GetDefaultProxyAddress();
static const char* SettingName(OptionsModel::OptionID option)
{
    switch (option) {
    case OptionsModel::DatabaseCache: return "dbcache";
    case OptionsModel::ThreadsScriptVerif: return "par";
    case OptionsModel::SpendZeroConfChange: return "spendzeroconfchange";
    case OptionsModel::ExternalSignerPath: return "signer";
    case OptionsModel::MapPortUPnP: return "upnp";
    case OptionsModel::MapPortNatpmp: return "natpmp";
    case OptionsModel::Listen: return "listen";
    case OptionsModel::Server: return "server";
    case OptionsModel::PruneSize: return "prune";
    case OptionsModel::Prune: return "prune";
    case OptionsModel::ProxyIP: return "proxy";
    case OptionsModel::ProxyPort: return "proxy";
    case OptionsModel::ProxyUse: return "proxy";
    case OptionsModel::ProxyIPTor: return "onion";
    case OptionsModel::ProxyPortTor: return "onion";
    case OptionsModel::ProxyUseTor: return "onion";
    case OptionsModel::Language: return "lang";
    default: throw std::logic_error(strprintf("GUI option %i has no corresponding node setting.", option));
    }
}
static void UpdateRwSetting(interfaces::Node& node, OptionsModel::OptionID option, const util::SettingsValue& value)
{
    if (value.isNum() &&
        (option == OptionsModel::DatabaseCache ||
         option == OptionsModel::ThreadsScriptVerif ||
         option == OptionsModel::Prune ||
         option == OptionsModel::PruneSize)) {
        node.updateRwSetting(SettingName(option), value.getValStr());
    } else {
        node.updateRwSetting(SettingName(option), value);
    }
}
static util::SettingsValue PruneSetting(bool prune_enabled, int prune_size_gb)
{
    assert(!prune_enabled || prune_size_gb >= 1);
    return prune_enabled ? PruneGBtoMiB(prune_size_gb) : 0;
}
static bool PruneEnabled(const util::SettingsValue& prune_setting)
{
    return SettingToInt(prune_setting, 0) > 1;
}
static int PruneSizeGB(const util::SettingsValue& prune_setting)
{
    int value = SettingToInt(prune_setting, 0);
    return value > 1 ? PruneMiBtoGB(value) : DEFAULT_PRUNE_TARGET_GB;
}
static int ParsePruneSizeGB(const QVariant& prune_size)
{
    return std::max(1, prune_size.toInt());
}
struct ProxySetting {
    bool is_set;
    QString ip;
    QString port;
};
static ProxySetting ParseProxyString(const std::string& proxy);
static std::string ProxyString(bool is_set, QString ip, QString port);
OptionsModel::OptionsModel(interfaces::Node& node, QObject *parent) :
    QAbstractListModel(parent), m_node{node}
{
}
void OptionsModel::addOverriddenOption(const std::string &option)
{
    strOverriddenByCommandLine += QString::fromStdString(option) + "=" + QString::fromStdString(gArgs.GetArg(option, "")) + " ";
}
bool OptionsModel::Init(bilingual_str& error)
{
    m_prune_size_gb = PruneSizeGB(node().getPersistentSetting("prune"));
    ProxySetting proxy = ParseProxyString(SettingToString(node().getPersistentSetting("proxy"), GetDefaultProxyAddress().toStdString()));
    m_proxy_ip = proxy.ip;
    m_proxy_port = proxy.port;
    ProxySetting onion = ParseProxyString(SettingToString(node().getPersistentSetting("onion"), GetDefaultProxyAddress().toStdString()));
    m_onion_ip = onion.ip;
    m_onion_port = onion.port;
    language = QString::fromStdString(SettingToString(node().getPersistentSetting("lang"), ""));
    checkAndMigrate();
    QSettings settings;
    setRestartRequired(false);
    if (!settings.contains("fHideTrayIcon")) {
        settings.setValue("fHideTrayIcon", false);
    }
    m_show_tray_icon = !settings.value("fHideTrayIcon").toBool();
    Q_EMIT showTrayIconChanged(m_show_tray_icon);
    if (!settings.contains("fMinimizeToTray"))
        settings.setValue("fMinimizeToTray", false);
    fMinimizeToTray = settings.value("fMinimizeToTray").toBool() && m_show_tray_icon;
    if (!settings.contains("fMinimizeOnClose"))
        settings.setValue("fMinimizeOnClose", false);
    fMinimizeOnClose = settings.value("fMinimizeOnClose").toBool();
    if (!settings.contains("DisplayLcRypUnit")) {
        settings.setValue("DisplayLcRypUnit", QVariant::fromValue(LcRypUnit::LCR));
    }
    QVariant unit = settings.value("DisplayLcRypUnit");
    if (unit.canConvert<LcRypUnit>()) {
        m_display_lcryp_unit = unit.value<LcRypUnit>();
    } else {
        m_display_lcryp_unit = LcRypUnit::LCR;
        settings.setValue("DisplayLcRypUnit", QVariant::fromValue(m_display_lcryp_unit));
    }
    if (!settings.contains("strThirdPartyTxUrls"))
        settings.setValue("strThirdPartyTxUrls", "");
    strThirdPartyTxUrls = settings.value("strThirdPartyTxUrls", "").toString();
    if (!settings.contains("fCoinControlFeatures"))
        settings.setValue("fCoinControlFeatures", false);
    fCoinControlFeatures = settings.value("fCoinControlFeatures", false).toBool();
    if (!settings.contains("enable_psbt_controls")) {
        settings.setValue("enable_psbt_controls", false);
    }
    m_enable_psbt_controls = settings.value("enable_psbt_controls", false).toBool();
    for (OptionID option : {DatabaseCache, ThreadsScriptVerif, SpendZeroConfChange, ExternalSignerPath, MapPortUPnP,
                            MapPortNatpmp, Listen, Server, Prune, ProxyUse, ProxyUseTor, Language}) {
        std::string setting = SettingName(option);
        if (node().isSettingIgnored(setting)) addOverriddenOption("-" + setting);
        try {
            getOption(option);
        } catch (const std::exception& e) {
            error.original = strprintf("Could not read setting \"%s\", %s.", setting, e.what());
            error.translated = tr("Could not read setting \"%1\", %2.").arg(QString::fromStdString(setting), e.what()).toStdString();
            return false;
        }
    }
    if (!settings.contains("strDataDir"))
        settings.setValue("strDataDir", GUIUtil::getDefaultDataDirectory());
#ifdef ENABLE_WALLET
    if (!settings.contains("SubFeeFromAmount")) {
        settings.setValue("SubFeeFromAmount", false);
    }
    m_sub_fee_from_amount = settings.value("SubFeeFromAmount", false).toBool();
#endif
    if (!settings.contains("UseEmbeddedMonospacedFont")) {
        settings.setValue("UseEmbeddedMonospacedFont", "true");
    }
    m_use_embedded_monospaced_font = settings.value("UseEmbeddedMonospacedFont").toBool();
    Q_EMIT useEmbeddedMonospacedFontChanged(m_use_embedded_monospaced_font);
    return true;
}
static void CopySettings(QSettings& dst, const QSettings& src)
{
    for (const QString& key : src.allKeys()) {
        dst.setValue(key, src.value(key));
    }
}
static void BackupSettings(const fs::path& filename, const QSettings& src)
{
    qInfo() << "Backing up GUI settings to" << GUIUtil::PathToQString(filename);
    QSettings dst(GUIUtil::PathToQString(filename), QSettings::IniFormat);
    dst.clear();
    CopySettings(dst, src);
}
void OptionsModel::Reset()
{
    node().resetSettings();
    QSettings settings;
    BackupSettings(gArgs.GetDataDirNet() / "guisettings.ini.bak", settings);
    QString dataDir = GUIUtil::getDefaultDataDirectory();
    dataDir = settings.value("strDataDir", dataDir).toString();
    settings.clear();
    settings.setValue("strDataDir", dataDir);
    settings.setValue("fReset", true);
    if (GUIUtil::GetStartOnSystemStartup())
        GUIUtil::SetStartOnSystemStartup(false);
}
int OptionsModel::rowCount(const QModelIndex & parent) const
{
    return OptionIDRowCount;
}
static ProxySetting ParseProxyString(const QString& proxy)
{
    static const ProxySetting default_val = {false, DEFAULT_GUI_PROXY_HOST, QString("%1").arg(DEFAULT_GUI_PROXY_PORT)};
    if (proxy.isEmpty()) {
        return default_val;
    }
    QStringList ip_port = GUIUtil::SplitSkipEmptyParts(proxy, ":");
    if (ip_port.size() == 2) {
        return {true, ip_port.at(0), ip_port.at(1)};
    } else {
        return default_val;
    }
}
static ProxySetting ParseProxyString(const std::string& proxy)
{
    return ParseProxyString(QString::fromStdString(proxy));
}
static std::string ProxyString(bool is_set, QString ip, QString port)
{
    return is_set ? QString(ip + ":" + port).toStdString() : "";
}
static const QString GetDefaultProxyAddress()
{
    return QString("%1:%2").arg(DEFAULT_GUI_PROXY_HOST).arg(DEFAULT_GUI_PROXY_PORT);
}
void OptionsModel::SetPruneTargetGB(int prune_target_gb)
{
    const util::SettingsValue cur_value = node().getPersistentSetting("prune");
    const util::SettingsValue new_value = PruneSetting(prune_target_gb > 0, prune_target_gb);
    m_prune_size_gb = prune_target_gb;
    node().forceSetting("prune", new_value);
    if (PruneEnabled(cur_value) != PruneEnabled(new_value) ||
        PruneSizeGB(cur_value) != PruneSizeGB(new_value)) {
        UpdateRwSetting(node(), Prune, new_value);
    }
}
QVariant OptionsModel::data(const QModelIndex & index, int role) const
{
    if(role == Qt::EditRole)
    {
        return getOption(OptionID(index.row()));
    }
    return QVariant();
}
bool OptionsModel::setData(const QModelIndex & index, const QVariant & value, int role)
{
    bool successful = true;
    if(role == Qt::EditRole)
    {
        successful = setOption(OptionID(index.row()), value);
    }
    Q_EMIT dataChanged(index, index);
    return successful;
}
QVariant OptionsModel::getOption(OptionID option) const
{
    auto setting = [&]{ return node().getPersistentSetting(SettingName(option)); };
    QSettings settings;
    switch (option) {
    case StartAtStartup:
        return GUIUtil::GetStartOnSystemStartup();
    case ShowTrayIcon:
        return m_show_tray_icon;
    case MinimizeToTray:
        return fMinimizeToTray;
    case MapPortUPnP:
#ifdef USE_UPNP
        return SettingToBool(setting(), DEFAULT_UPNP);
#else
        return false;
#endif
    case MapPortNatpmp:
#ifdef USE_NATPMP
        return SettingToBool(setting(), DEFAULT_NATPMP);
#else
        return false;
#endif
    case MinimizeOnClose:
        return fMinimizeOnClose;
    case ProxyUse:
        return ParseProxyString(SettingToString(setting(), "")).is_set;
    case ProxyIP:
        return m_proxy_ip;
    case ProxyPort:
        return m_proxy_port;
    case ProxyUseTor:
        return ParseProxyString(SettingToString(setting(), "")).is_set;
    case ProxyIPTor:
        return m_onion_ip;
    case ProxyPortTor:
        return m_onion_port;
#ifdef ENABLE_WALLET
    case SpendZeroConfChange:
        return SettingToBool(setting(), wallet::DEFAULT_SPEND_ZEROCONF_CHANGE);
    case ExternalSignerPath:
        return QString::fromStdString(SettingToString(setting(), ""));
    case SubFeeFromAmount:
        return m_sub_fee_from_amount;
#endif
    case DisplayUnit:
        return QVariant::fromValue(m_display_lcryp_unit);
    case ThirdPartyTxUrls:
        return strThirdPartyTxUrls;
    case Language:
        return QString::fromStdString(SettingToString(setting(), ""));
    case UseEmbeddedMonospacedFont:
        return m_use_embedded_monospaced_font;
    case CoinControlFeatures:
        return fCoinControlFeatures;
    case EnablePSBTControls:
        return settings.value("enable_psbt_controls");
    case Prune:
        return PruneEnabled(setting());
    case PruneSize:
        return m_prune_size_gb;
    case DatabaseCache:
        return qlonglong(SettingToInt(setting(), nDefaultDbCache));
    case ThreadsScriptVerif:
        return qlonglong(SettingToInt(setting(), DEFAULT_SCRIPTCHECK_THREADS));
    case Listen:
        return SettingToBool(setting(), DEFAULT_LISTEN);
    case Server:
        return SettingToBool(setting(), false);
    default:
        return QVariant();
    }
}
bool OptionsModel::setOption(OptionID option, const QVariant& value)
{
    auto changed = [&] { return value.isValid() && value != getOption(option); };
    auto update = [&](const util::SettingsValue& value) { return UpdateRwSetting(node(), option, value); };
    bool successful = true;
    QSettings settings;
    switch (option) {
    case StartAtStartup:
        successful = GUIUtil::SetStartOnSystemStartup(value.toBool());
        break;
    case ShowTrayIcon:
        m_show_tray_icon = value.toBool();
        settings.setValue("fHideTrayIcon", !m_show_tray_icon);
        Q_EMIT showTrayIconChanged(m_show_tray_icon);
        break;
    case MinimizeToTray:
        fMinimizeToTray = value.toBool();
        settings.setValue("fMinimizeToTray", fMinimizeToTray);
        break;
    case MapPortUPnP:
        if (changed()) {
            update(value.toBool());
            node().mapPort(value.toBool(), getOption(MapPortNatpmp).toBool());
        }
        break;
    case MapPortNatpmp:
        if (changed()) {
            update(value.toBool());
            node().mapPort(getOption(MapPortUPnP).toBool(), value.toBool());
        }
        break;
    case MinimizeOnClose:
        fMinimizeOnClose = value.toBool();
        settings.setValue("fMinimizeOnClose", fMinimizeOnClose);
        break;
    case ProxyUse:
        if (changed()) {
            update(ProxyString(value.toBool(), m_proxy_ip, m_proxy_port));
            setRestartRequired(true);
        }
        break;
    case ProxyIP:
        if (changed()) {
            m_proxy_ip = value.toString();
            if (getOption(ProxyUse).toBool()) {
                update(ProxyString(true, m_proxy_ip, m_proxy_port));
                setRestartRequired(true);
            }
        }
        break;
    case ProxyPort:
        if (changed()) {
            m_proxy_port = value.toString();
            if (getOption(ProxyUse).toBool()) {
                update(ProxyString(true, m_proxy_ip, m_proxy_port));
                setRestartRequired(true);
            }
        }
        break;
    case ProxyUseTor:
        if (changed()) {
            update(ProxyString(value.toBool(), m_onion_ip, m_onion_port));
            setRestartRequired(true);
        }
        break;
    case ProxyIPTor:
        if (changed()) {
            m_onion_ip = value.toString();
            if (getOption(ProxyUseTor).toBool()) {
                update(ProxyString(true, m_onion_ip, m_onion_port));
                setRestartRequired(true);
            }
        }
        break;
    case ProxyPortTor:
        if (changed()) {
            m_onion_port = value.toString();
            if (getOption(ProxyUseTor).toBool()) {
                update(ProxyString(true, m_onion_ip, m_onion_port));
                setRestartRequired(true);
            }
        }
        break;
#ifdef ENABLE_WALLET
    case SpendZeroConfChange:
        if (changed()) {
            update(value.toBool());
            setRestartRequired(true);
        }
        break;
    case ExternalSignerPath:
        if (changed()) {
            update(value.toString().toStdString());
            setRestartRequired(true);
        }
        break;
    case SubFeeFromAmount:
        m_sub_fee_from_amount = value.toBool();
        settings.setValue("SubFeeFromAmount", m_sub_fee_from_amount);
        break;
#endif
    case DisplayUnit:
        setDisplayUnit(value);
        break;
    case ThirdPartyTxUrls:
        if (strThirdPartyTxUrls != value.toString()) {
            strThirdPartyTxUrls = value.toString();
            settings.setValue("strThirdPartyTxUrls", strThirdPartyTxUrls);
            setRestartRequired(true);
        }
        break;
    case Language:
        if (changed()) {
            update(value.toString().toStdString());
            setRestartRequired(true);
        }
        break;
    case UseEmbeddedMonospacedFont:
        m_use_embedded_monospaced_font = value.toBool();
        settings.setValue("UseEmbeddedMonospacedFont", m_use_embedded_monospaced_font);
        Q_EMIT useEmbeddedMonospacedFontChanged(m_use_embedded_monospaced_font);
        break;
    case CoinControlFeatures:
        fCoinControlFeatures = value.toBool();
        settings.setValue("fCoinControlFeatures", fCoinControlFeatures);
        Q_EMIT coinControlFeaturesChanged(fCoinControlFeatures);
        break;
    case EnablePSBTControls:
        m_enable_psbt_controls = value.toBool();
        settings.setValue("enable_psbt_controls", m_enable_psbt_controls);
        break;
    case Prune:
        if (changed()) {
            update(PruneSetting(value.toBool(), m_prune_size_gb));
            setRestartRequired(true);
        }
        break;
    case PruneSize:
        if (changed()) {
            m_prune_size_gb = ParsePruneSizeGB(value);
            if (getOption(Prune).toBool()) {
                update(PruneSetting(true, m_prune_size_gb));
                setRestartRequired(true);
            }
        }
        break;
    case DatabaseCache:
        if (changed()) {
            update(static_cast<int64_t>(value.toLongLong()));
            setRestartRequired(true);
        }
        break;
    case ThreadsScriptVerif:
        if (changed()) {
            update(static_cast<int64_t>(value.toLongLong()));
            setRestartRequired(true);
        }
        break;
    case Listen:
    case Server:
        if (changed()) {
            update(value.toBool());
            setRestartRequired(true);
        }
        break;
    default:
        break;
    }
    return successful;
}
void OptionsModel::setDisplayUnit(const QVariant& new_unit)
{
    if (new_unit.isNull() || new_unit.value<LcRypUnit>() == m_display_lcryp_unit) return;
    m_display_lcryp_unit = new_unit.value<LcRypUnit>();
    QSettings settings;
    settings.setValue("DisplayLcRypUnit", QVariant::fromValue(m_display_lcryp_unit));
    Q_EMIT displayUnitChanged(m_display_lcryp_unit);
}
void OptionsModel::setRestartRequired(bool fRequired)
{
    QSettings settings;
    return settings.setValue("fRestartRequired", fRequired);
}
bool OptionsModel::isRestartRequired() const
{
    QSettings settings;
    return settings.value("fRestartRequired", false).toBool();
}
void OptionsModel::checkAndMigrate()
{
    QSettings settings;
    static const char strSettingsVersionKey[] = "nSettingsVersion";
    int settingsVersion = settings.contains(strSettingsVersionKey) ? settings.value(strSettingsVersionKey).toInt() : 0;
    if (settingsVersion < CLIENT_VERSION)
    {
        if (settingsVersion < 130000 && settings.contains("nDatabaseCache") && settings.value("nDatabaseCache").toLongLong() == 100)
            settings.setValue("nDatabaseCache", (qint64)nDefaultDbCache);
        settings.setValue(strSettingsVersionKey, CLIENT_VERSION);
    }
    if (settings.contains("addrProxy") && settings.value("addrProxy").toString().endsWith("%2")) {
        settings.setValue("addrProxy", GetDefaultProxyAddress());
    }
    if (settings.contains("addrSeparateProxyTor") && settings.value("addrSeparateProxyTor").toString().endsWith("%2")) {
        settings.setValue("addrSeparateProxyTor", GetDefaultProxyAddress());
    }
    auto migrate_setting = [&](OptionID option, const QString& qt_name) {
        if (!settings.contains(qt_name)) return;
        QVariant value = settings.value(qt_name);
        if (node().getPersistentSetting(SettingName(option)).isNull()) {
            if (option == ProxyIP) {
                ProxySetting parsed = ParseProxyString(value.toString());
                setOption(ProxyIP, parsed.ip);
                setOption(ProxyPort, parsed.port);
            } else if (option == ProxyIPTor) {
                ProxySetting parsed = ParseProxyString(value.toString());
                setOption(ProxyIPTor, parsed.ip);
                setOption(ProxyPortTor, parsed.port);
            } else {
                setOption(option, value);
            }
        }
        settings.remove(qt_name);
    };
    migrate_setting(DatabaseCache, "nDatabaseCache");
    migrate_setting(ThreadsScriptVerif, "nThreadsScriptVerif");
#ifdef ENABLE_WALLET
    migrate_setting(SpendZeroConfChange, "bSpendZeroConfChange");
    migrate_setting(ExternalSignerPath, "external_signer_path");
#endif
    migrate_setting(MapPortUPnP, "fUseUPnP");
    migrate_setting(MapPortNatpmp, "fUseNatpmp");
    migrate_setting(Listen, "fListen");
    migrate_setting(Server, "server");
    migrate_setting(PruneSize, "nPruneSize");
    migrate_setting(Prune, "bPrune");
    migrate_setting(ProxyIP, "addrProxy");
    migrate_setting(ProxyUse, "fUseProxy");
    migrate_setting(ProxyIPTor, "addrSeparateProxyTor");
    migrate_setting(ProxyUseTor, "fUseSeparateProxyTor");
    migrate_setting(Language, "language");
    node().initParameterInteraction();
}
