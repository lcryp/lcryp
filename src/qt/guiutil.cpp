#include <qt/guiutil.h>
#include <qt/lcrypaddressvalidator.h>
#include <qt/lcrypunits.h>
#include <qt/platformstyle.h>
#include <qt/qvalidatedlineedit.h>
#include <qt/sendcoinsrecipient.h>
#include <base58.h>
#include <chainparams.h>
#include <fs.h>
#include <interfaces/node.h>
#include <key_io.h>
#include <policy/policy.h>
#include <primitives/transaction.h>
#include <protocol.h>
#include <script/script.h>
#include <script/standard.h>
#include <util/system.h>
#include <util/time.h>
#ifdef WIN32
#include <shellapi.h>
#include <shlobj.h>
#include <shlwapi.h>
#endif
#include <QAbstractButton>
#include <QAbstractItemView>
#include <QApplication>
#include <QClipboard>
#include <QDateTime>
#include <QDesktopServices>
#include <QDialog>
#include <QDoubleValidator>
#include <QFileDialog>
#include <QFont>
#include <QFontDatabase>
#include <QFontMetrics>
#include <QGuiApplication>
#include <QJsonObject>
#include <QKeyEvent>
#include <QKeySequence>
#include <QLatin1String>
#include <QLineEdit>
#include <QList>
#include <QLocale>
#include <QMenu>
#include <QMouseEvent>
#include <QPluginLoader>
#include <QProgressDialog>
#include <QRegularExpression>
#include <QScreen>
#include <QSettings>
#include <QShortcut>
#include <QSize>
#include <QStandardPaths>
#include <QString>
#include <QTextDocument>
#include <QThread>
#include <QUrlQuery>
#include <QtGlobal>
#include <cassert>
#include <chrono>
#include <exception>
#include <fstream>
#include <string>
#include <vector>
#if defined(Q_OS_MACOS)
#include <QProcess>
void ForceActivation();
#endif
using namespace std::chrono_literals;
namespace GUIUtil {
QString dateTimeStr(const QDateTime &date)
{
    return QLocale::system().toString(date.date(), QLocale::ShortFormat) + QString(" ") + date.toString("hh:mm");
}
QString dateTimeStr(qint64 nTime)
{
    return dateTimeStr(QDateTime::fromSecsSinceEpoch(nTime));
}
QFont fixedPitchFont(bool use_embedded_font)
{
    if (use_embedded_font) {
        return {"Roboto Mono"};
    }
    return QFontDatabase::systemFont(QFontDatabase::FixedFont);
}
static const uint8_t dummydata[] = {0xeb,0x15,0x23,0x1d,0xfc,0xeb,0x60,0x92,0x58,0x86,0xb6,0x7d,0x06,0x52,0x99,0x92,0x59,0x15,0xae,0xb1,0x72,0xc0,0x66,0x47};
static std::string DummyAddress(const CChainParams &params)
{
    std::vector<unsigned char> sourcedata = params.Base58Prefix(CChainParams::PUBKEY_ADDRESS);
    sourcedata.insert(sourcedata.end(), dummydata, dummydata + sizeof(dummydata));
    for(int i=0; i<256; ++i) {
        std::string s = EncodeBase58(sourcedata);
        if (!IsValidDestinationString(s)) {
            return s;
        }
        sourcedata[sourcedata.size()-1] += 1;
    }
    return "";
}
void setupAddressWidget(QValidatedLineEdit *widget, QWidget *parent)
{
    parent->setFocusProxy(widget);
    widget->setFont(fixedPitchFont());
    widget->setPlaceholderText(QObject::tr("Enter a LcRyp address (e.g. %1)").arg(
        QString::fromStdString(DummyAddress(Params()))));
    widget->setValidator(new LcRypAddressEntryValidator(parent));
    widget->setCheckValidator(new LcRypAddressCheckValidator(parent));
}
void AddButtonShortcut(QAbstractButton* button, const QKeySequence& shortcut)
{
    QObject::connect(new QShortcut(shortcut, button), &QShortcut::activated, [button]() { button->animateClick(); });
}
bool parseLcRypURI(const QUrl &uri, SendCoinsRecipient *out)
{
    if(!uri.isValid() || uri.scheme() != QString("lcryp"))
        return false;
    SendCoinsRecipient rv;
    rv.address = uri.path();
    if (rv.address.endsWith("/")) {
        rv.address.truncate(rv.address.length() - 1);
    }
    rv.amount = 0;
    QUrlQuery uriQuery(uri);
    QList<QPair<QString, QString> > items = uriQuery.queryItems();
    for (QList<QPair<QString, QString> >::iterator i = items.begin(); i != items.end(); i++)
    {
        bool fShouldReturnFalse = false;
        if (i->first.startsWith("req-"))
        {
            i->first.remove(0, 4);
            fShouldReturnFalse = true;
        }
        if (i->first == "label")
        {
            rv.label = i->second;
            fShouldReturnFalse = false;
        }
        if (i->first == "message")
        {
            rv.message = i->second;
            fShouldReturnFalse = false;
        }
        else if (i->first == "amount")
        {
            if(!i->second.isEmpty())
            {
                if (!LcRypUnits::parse(LcRypUnit::LCR, i->second, &rv.amount)) {
                    return false;
                }
            }
            fShouldReturnFalse = false;
        }
        if (fShouldReturnFalse)
            return false;
    }
    if(out)
    {
        *out = rv;
    }
    return true;
}
bool parseLcRypURI(QString uri, SendCoinsRecipient *out)
{
    QUrl uriInstance(uri);
    return parseLcRypURI(uriInstance, out);
}
QString formatLcRypURI(const SendCoinsRecipient &info)
{
    bool bech_32 = info.address.startsWith(QString::fromStdString(Params().Bech32HRP() + "1"));
    QString ret = QString("lcryp:%1").arg(bech_32 ? info.address.toUpper() : info.address);
    int paramCount = 0;
    if (info.amount)
    {
        ret += QString("?amount=%1").arg(LcRypUnits::format(LcRypUnit::LCR, info.amount, false, LcRypUnits::SeparatorStyle::NEVER));
        paramCount++;
    }
    if (!info.label.isEmpty())
    {
        QString lbl(QUrl::toPercentEncoding(info.label));
        ret += QString("%1label=%2").arg(paramCount == 0 ? "?" : "&").arg(lbl);
        paramCount++;
    }
    if (!info.message.isEmpty())
    {
        QString msg(QUrl::toPercentEncoding(info.message));
        ret += QString("%1message=%2").arg(paramCount == 0 ? "?" : "&").arg(msg);
        paramCount++;
    }
    return ret;
}
bool isDust(interfaces::Node& node, const QString& address, const CAmount& amount)
{
    CTxDestination dest = DecodeDestination(address.toStdString());
    CScript script = GetScriptForDestination(dest);
    CTxOut txOut(amount, script);
    return IsDust(txOut, node.getDustRelayFee());
}
QString HtmlEscape(const QString& str, bool fMultiLine)
{
    QString escaped = str.toHtmlEscaped();
    if(fMultiLine)
    {
        escaped = escaped.replace("\n", "<br>\n");
    }
    return escaped;
}
QString HtmlEscape(const std::string& str, bool fMultiLine)
{
    return HtmlEscape(QString::fromStdString(str), fMultiLine);
}
void copyEntryData(const QAbstractItemView *view, int column, int role)
{
    if(!view || !view->selectionModel())
        return;
    QModelIndexList selection = view->selectionModel()->selectedRows(column);
    if(!selection.isEmpty())
    {
        setClipboard(selection.at(0).data(role).toString());
    }
}
QList<QModelIndex> getEntryData(const QAbstractItemView *view, int column)
{
    if(!view || !view->selectionModel())
        return QList<QModelIndex>();
    return view->selectionModel()->selectedRows(column);
}
bool hasEntryData(const QAbstractItemView *view, int column, int role)
{
    QModelIndexList selection = getEntryData(view, column);
    if (selection.isEmpty()) return false;
    return !selection.at(0).data(role).toString().isEmpty();
}
void LoadFont(const QString& file_name)
{
    const int id = QFontDatabase::addApplicationFont(file_name);
    assert(id != -1);
}
QString getDefaultDataDirectory()
{
    return PathToQString(GetDefaultDataDir());
}
QString ExtractFirstSuffixFromFilter(const QString& filter)
{
    QRegularExpression filter_re(QStringLiteral(".* \\(\\*\\.(.*)[ \\)]"), QRegularExpression::InvertedGreedinessOption);
    QString suffix;
    QRegularExpressionMatch m = filter_re.match(filter);
    if (m.hasMatch()) {
        suffix = m.captured(1);
    }
    return suffix;
}
QString getSaveFileName(QWidget *parent, const QString &caption, const QString &dir,
    const QString &filter,
    QString *selectedSuffixOut)
{
    QString selectedFilter;
    QString myDir;
    if(dir.isEmpty())
    {
        myDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    }
    else
    {
        myDir = dir;
    }
    QString result = QDir::toNativeSeparators(QFileDialog::getSaveFileName(parent, caption, myDir, filter, &selectedFilter));
    QString selectedSuffix = ExtractFirstSuffixFromFilter(selectedFilter);
    QFileInfo info(result);
    if(!result.isEmpty())
    {
        if(info.suffix().isEmpty() && !selectedSuffix.isEmpty())
        {
            if(!result.endsWith("."))
                result.append(".");
            result.append(selectedSuffix);
        }
    }
    if(selectedSuffixOut)
    {
        *selectedSuffixOut = selectedSuffix;
    }
    return result;
}
QString getOpenFileName(QWidget *parent, const QString &caption, const QString &dir,
    const QString &filter,
    QString *selectedSuffixOut)
{
    QString selectedFilter;
    QString myDir;
    if(dir.isEmpty())
    {
        myDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    }
    else
    {
        myDir = dir;
    }
    QString result = QDir::toNativeSeparators(QFileDialog::getOpenFileName(parent, caption, myDir, filter, &selectedFilter));
    if(selectedSuffixOut)
    {
        *selectedSuffixOut = ExtractFirstSuffixFromFilter(selectedFilter);
        ;
    }
    return result;
}
Qt::ConnectionType blockingGUIThreadConnection()
{
    if(QThread::currentThread() != qApp->thread())
    {
        return Qt::BlockingQueuedConnection;
    }
    else
    {
        return Qt::DirectConnection;
    }
}
bool checkPoint(const QPoint &p, const QWidget *w)
{
    QWidget *atW = QApplication::widgetAt(w->mapToGlobal(p));
    if (!atW) return false;
    return atW->window() == w;
}
bool isObscured(QWidget *w)
{
    return !(checkPoint(QPoint(0, 0), w)
        && checkPoint(QPoint(w->width() - 1, 0), w)
        && checkPoint(QPoint(0, w->height() - 1), w)
        && checkPoint(QPoint(w->width() - 1, w->height() - 1), w)
        && checkPoint(QPoint(w->width() / 2, w->height() / 2), w));
}
void bringToFront(QWidget* w)
{
#ifdef Q_OS_MACOS
    ForceActivation();
#endif
    if (w) {
        if (w->isMinimized()) {
            w->showNormal();
        } else {
            w->show();
        }
        w->activateWindow();
        w->raise();
    }
}
void handleCloseWindowShortcut(QWidget* w)
{
    QObject::connect(new QShortcut(QKeySequence(QObject::tr("Ctrl+W")), w), &QShortcut::activated, w, &QWidget::close);
}
void openDebugLogfile()
{
    fs::path pathDebug = gArgs.GetDataDirNet() / "debug.log";
    if (fs::exists(pathDebug))
        QDesktopServices::openUrl(QUrl::fromLocalFile(PathToQString(pathDebug)));
}
bool openLcRypConf()
{
    fs::path pathConfig = GetConfigFile(gArgs.GetPathArg("-conf", LCRYP_CONF_FILENAME));
    std::ofstream configFile{pathConfig, std::ios_base::app};
    if (!configFile.good())
        return false;
    configFile.close();
    bool res = QDesktopServices::openUrl(QUrl::fromLocalFile(PathToQString(pathConfig)));
#ifdef Q_OS_MACOS
    if (!res) {
        res = QProcess::startDetached("/usr/bin/open", QStringList{"-t", PathToQString(pathConfig)});
    }
#endif
    return res;
}
ToolTipToRichTextFilter::ToolTipToRichTextFilter(int _size_threshold, QObject *parent) :
    QObject(parent),
    size_threshold(_size_threshold)
{
}
bool ToolTipToRichTextFilter::eventFilter(QObject *obj, QEvent *evt)
{
    if(evt->type() == QEvent::ToolTipChange)
    {
        QWidget *widget = static_cast<QWidget*>(obj);
        QString tooltip = widget->toolTip();
        if(tooltip.size() > size_threshold && !tooltip.startsWith("<qt") && !Qt::mightBeRichText(tooltip))
        {
            tooltip = "<qt>" + HtmlEscape(tooltip, true) + "</qt>";
            widget->setToolTip(tooltip);
            return true;
        }
    }
    return QObject::eventFilter(obj, evt);
}
LabelOutOfFocusEventFilter::LabelOutOfFocusEventFilter(QObject* parent)
    : QObject(parent)
{
}
bool LabelOutOfFocusEventFilter::eventFilter(QObject* watched, QEvent* event)
{
    if (event->type() == QEvent::FocusOut) {
        auto focus_out = static_cast<QFocusEvent*>(event);
        if (focus_out->reason() != Qt::PopupFocusReason) {
            auto label = qobject_cast<QLabel*>(watched);
            if (label) {
                auto flags = label->textInteractionFlags();
                label->setTextInteractionFlags(Qt::NoTextInteraction);
                label->setTextInteractionFlags(flags);
            }
        }
    }
    return QObject::eventFilter(watched, event);
}
#ifdef WIN32
fs::path static StartupShortcutPath()
{
    std::string chain = gArgs.GetChainName();
    if (chain == CBaseChainParams::MAIN)
        return GetSpecialFolderPath(CSIDL_STARTUP) / "LcRyp.lnk";
    if (chain == CBaseChainParams::TESTNET)
        return GetSpecialFolderPath(CSIDL_STARTUP) / "LcRyp (testnet).lnk";
    return GetSpecialFolderPath(CSIDL_STARTUP) / fs::u8path(strprintf("LcRyp (%s).lnk", chain));
}
bool GetStartOnSystemStartup()
{
    return fs::exists(StartupShortcutPath());
}
bool SetStartOnSystemStartup(bool fAutoStart)
{
    fs::remove(StartupShortcutPath());
    if (fAutoStart)
    {
        CoInitialize(nullptr);
        IShellLinkW* psl = nullptr;
        HRESULT hres = CoCreateInstance(CLSID_ShellLink, nullptr,
            CLSCTX_INPROC_SERVER, IID_IShellLinkW,
            reinterpret_cast<void**>(&psl));
        if (SUCCEEDED(hres))
        {
            WCHAR pszExePath[MAX_PATH];
            GetModuleFileNameW(nullptr, pszExePath, ARRAYSIZE(pszExePath));
            QString strArgs = "-min";
            strArgs += QString::fromStdString(strprintf(" -chain=%s", gArgs.GetChainName()));
            psl->SetPath(pszExePath);
            PathRemoveFileSpecW(pszExePath);
            psl->SetWorkingDirectory(pszExePath);
            psl->SetShowCmd(SW_SHOWMINNOACTIVE);
            psl->SetArguments(strArgs.toStdWString().c_str());
            IPersistFile* ppf = nullptr;
            hres = psl->QueryInterface(IID_IPersistFile, reinterpret_cast<void**>(&ppf));
            if (SUCCEEDED(hres))
            {
                hres = ppf->Save(StartupShortcutPath().wstring().c_str(), TRUE);
                ppf->Release();
                psl->Release();
                CoUninitialize();
                return true;
            }
            psl->Release();
        }
        CoUninitialize();
        return false;
    }
    return true;
}
#elif defined(Q_OS_LINUX)
fs::path static GetAutostartDir()
{
    char* pszConfigHome = getenv("XDG_CONFIG_HOME");
    if (pszConfigHome) return fs::path(pszConfigHome) / "autostart";
    char* pszHome = getenv("HOME");
    if (pszHome) return fs::path(pszHome) / ".config" / "autostart";
    return fs::path();
}
fs::path static GetAutostartFilePath()
{
    std::string chain = gArgs.GetChainName();
    if (chain == CBaseChainParams::MAIN)
        return GetAutostartDir() / "lcryp.desktop";
    return GetAutostartDir() / fs::u8path(strprintf("lcryp-%s.desktop", chain));
}
bool GetStartOnSystemStartup()
{
    std::ifstream optionFile{GetAutostartFilePath()};
    if (!optionFile.good())
        return false;
    std::string line;
    while (!optionFile.eof())
    {
        getline(optionFile, line);
        if (line.find("Hidden") != std::string::npos &&
            line.find("true") != std::string::npos)
            return false;
    }
    optionFile.close();
    return true;
}
bool SetStartOnSystemStartup(bool fAutoStart)
{
    if (!fAutoStart)
        fs::remove(GetAutostartFilePath());
    else
    {
        char pszExePath[MAX_PATH+1];
        ssize_t r = readlink("/proc/self/exe", pszExePath, sizeof(pszExePath) - 1);
        if (r == -1)
            return false;
        pszExePath[r] = '\0';
        fs::create_directories(GetAutostartDir());
        std::ofstream optionFile{GetAutostartFilePath(), std::ios_base::out | std::ios_base::trunc};
        if (!optionFile.good())
            return false;
        std::string chain = gArgs.GetChainName();
        optionFile << "[Desktop Entry]\n";
        optionFile << "Type=Application\n";
        if (chain == CBaseChainParams::MAIN)
            optionFile << "Name=LcRyp\n";
        else
            optionFile << strprintf("Name=LcRyp (%s)\n", chain);
        optionFile << "Exec=" << pszExePath << strprintf(" -min -chain=%s\n", chain);
        optionFile << "Terminal=false\n";
        optionFile << "Hidden=false\n";
        optionFile.close();
    }
    return true;
}
#else
bool GetStartOnSystemStartup() { return false; }
bool SetStartOnSystemStartup(bool fAutoStart) { return false; }
#endif
void setClipboard(const QString& str)
{
    QClipboard* clipboard = QApplication::clipboard();
    clipboard->setText(str, QClipboard::Clipboard);
    if (clipboard->supportsSelection()) {
        clipboard->setText(str, QClipboard::Selection);
    }
}
fs::path QStringToPath(const QString &path)
{
    return fs::u8path(path.toStdString());
}
QString PathToQString(const fs::path &path)
{
    return QString::fromStdString(path.u8string());
}
QString NetworkToQString(Network net)
{
    switch (net) {
    case NET_UNROUTABLE: return QObject::tr("Unroutable");
    case NET_IPV4: return "IPv4";
    case NET_IPV6: return "IPv6";
    case NET_ONION: return "Onion";
    case NET_I2P: return "I2P";
    case NET_CJDNS: return "CJDNS";
    case NET_INTERNAL: return QObject::tr("Internal");
    case NET_MAX: assert(false);
    }
    assert(false);
}
QString ConnectionTypeToQString(ConnectionType conn_type, bool prepend_direction)
{
    QString prefix;
    if (prepend_direction) {
        prefix = (conn_type == ConnectionType::INBOUND) ?
                     QObject::tr("Inbound") :
                     QObject::tr("Outbound") + " ";
    }
    switch (conn_type) {
    case ConnectionType::INBOUND: return prefix;
    case ConnectionType::OUTBOUND_FULL_RELAY: return prefix + QObject::tr("Full Relay");
    case ConnectionType::BLOCK_RELAY: return prefix + QObject::tr("Block Relay");
    case ConnectionType::MANUAL: return prefix + QObject::tr("Manual");
    case ConnectionType::FEELER: return prefix + QObject::tr("Feeler");
    case ConnectionType::ADDR_FETCH: return prefix + QObject::tr("Address Fetch");
    }
    assert(false);
}
QString formatDurationStr(std::chrono::seconds dur)
{
    using days = std::chrono::duration<int, std::ratio<86400>>;
    const auto d{std::chrono::duration_cast<days>(dur)};
    const auto h{std::chrono::duration_cast<std::chrono::hours>(dur - d)};
    const auto m{std::chrono::duration_cast<std::chrono::minutes>(dur - d - h)};
    const auto s{std::chrono::duration_cast<std::chrono::seconds>(dur - d - h - m)};
    QStringList str_list;
    if (auto d2{d.count()}) str_list.append(QObject::tr("%1 d").arg(d2));
    if (auto h2{h.count()}) str_list.append(QObject::tr("%1 h").arg(h2));
    if (auto m2{m.count()}) str_list.append(QObject::tr("%1 m").arg(m2));
    const auto s2{s.count()};
    if (s2 || str_list.empty()) str_list.append(QObject::tr("%1 s").arg(s2));
    return str_list.join(" ");
}
QString FormatPeerAge(std::chrono::seconds time_connected)
{
    const auto time_now{GetTime<std::chrono::seconds>()};
    const auto age{time_now - time_connected};
    if (age >= 24h) return QObject::tr("%1 d").arg(age / 24h);
    if (age >= 1h) return QObject::tr("%1 h").arg(age / 1h);
    if (age >= 1min) return QObject::tr("%1 m").arg(age / 1min);
    return QObject::tr("%1 s").arg(age / 1s);
}
QString formatServicesStr(quint64 mask)
{
    QStringList strList;
    for (const auto& flag : serviceFlagsToStr(mask)) {
        strList.append(QString::fromStdString(flag));
    }
    if (strList.size())
        return strList.join(", ");
    else
        return QObject::tr("None");
}
QString formatPingTime(std::chrono::microseconds ping_time)
{
    return (ping_time == std::chrono::microseconds::max() || ping_time == 0us) ?
        QObject::tr("N/A") :
        QObject::tr("%1 ms").arg(QString::number((int)(count_microseconds(ping_time) / 1000), 10));
}
QString formatTimeOffset(int64_t nTimeOffset)
{
  return QObject::tr("%1 s").arg(QString::number((int)nTimeOffset, 10));
}
QString formatNiceTimeOffset(qint64 secs)
{
    QString timeBehindText;
    const int HOUR_IN_SECONDS = 60*60;
    const int DAY_IN_SECONDS = 24*60*60;
    const int WEEK_IN_SECONDS = 7*24*60*60;
    const int YEAR_IN_SECONDS = 31556952;
    if(secs < 60)
    {
        timeBehindText = QObject::tr("%n second(s)","",secs);
    }
    else if(secs < 2*HOUR_IN_SECONDS)
    {
        timeBehindText = QObject::tr("%n minute(s)","",secs/60);
    }
    else if(secs < 2*DAY_IN_SECONDS)
    {
        timeBehindText = QObject::tr("%n hour(s)","",secs/HOUR_IN_SECONDS);
    }
    else if(secs < 2*WEEK_IN_SECONDS)
    {
        timeBehindText = QObject::tr("%n day(s)","",secs/DAY_IN_SECONDS);
    }
    else if(secs < YEAR_IN_SECONDS)
    {
        timeBehindText = QObject::tr("%n week(s)","",secs/WEEK_IN_SECONDS);
    }
    else
    {
        qint64 years = secs / YEAR_IN_SECONDS;
        qint64 remainder = secs % YEAR_IN_SECONDS;
        timeBehindText = QObject::tr("%1 and %2").arg(QObject::tr("%n year(s)", "", years)).arg(QObject::tr("%n week(s)","", remainder/WEEK_IN_SECONDS));
    }
    return timeBehindText;
}
QString formatBytes(uint64_t bytes)
{
    if (bytes < 1'000)
        return QObject::tr("%1 B").arg(bytes);
    if (bytes < 1'000'000)
        return QObject::tr("%1 kB").arg(bytes / 1'000);
    if (bytes < 1'000'000'000)
        return QObject::tr("%1 MB").arg(bytes / 1'000'000);
    return QObject::tr("%1 GB").arg(bytes / 1'000'000'000);
}
qreal calculateIdealFontSize(int width, const QString& text, QFont font, qreal minPointSize, qreal font_size) {
    while(font_size >= minPointSize) {
        font.setPointSizeF(font_size);
        QFontMetrics fm(font);
        if (TextWidth(fm, text) < width) {
            break;
        }
        font_size -= 0.5;
    }
    return font_size;
}
ThemedLabel::ThemedLabel(const PlatformStyle* platform_style, QWidget* parent)
    : QLabel{parent}, m_platform_style{platform_style}
{
    assert(m_platform_style);
}
void ThemedLabel::setThemedPixmap(const QString& image_filename, int width, int height)
{
    m_image_filename = image_filename;
    m_pixmap_width = width;
    m_pixmap_height = height;
    updateThemedPixmap();
}
void ThemedLabel::changeEvent(QEvent* e)
{
    if (e->type() == QEvent::PaletteChange) {
        updateThemedPixmap();
    }
    QLabel::changeEvent(e);
}
void ThemedLabel::updateThemedPixmap()
{
    setPixmap(m_platform_style->SingleColorIcon(m_image_filename).pixmap(m_pixmap_width, m_pixmap_height));
}
ClickableLabel::ClickableLabel(const PlatformStyle* platform_style, QWidget* parent)
    : ThemedLabel{platform_style, parent}
{
}
void ClickableLabel::mouseReleaseEvent(QMouseEvent *event)
{
    Q_EMIT clicked(event->pos());
}
void ClickableProgressBar::mouseReleaseEvent(QMouseEvent *event)
{
    Q_EMIT clicked(event->pos());
}
bool ItemDelegate::eventFilter(QObject *object, QEvent *event)
{
    if (event->type() == QEvent::KeyPress) {
        if (static_cast<QKeyEvent*>(event)->key() == Qt::Key_Escape) {
            Q_EMIT keyEscapePressed();
        }
    }
    return QItemDelegate::eventFilter(object, event);
}
void PolishProgressDialog(QProgressDialog* dialog)
{
#ifdef Q_OS_MACOS
    const int margin = TextWidth(dialog->fontMetrics(), ("X"));
    dialog->resize(dialog->width() + 2 * margin, dialog->height());
#endif
    dialog->setMinimumDuration(0);
}
int TextWidth(const QFontMetrics& fm, const QString& text)
{
    return fm.horizontalAdvance(text);
}
void LogQtInfo()
{
#ifdef QT_STATIC
    const std::string qt_link{"static"};
#else
    const std::string qt_link{"dynamic"};
#endif
#ifdef QT_STATICPLUGIN
    const std::string plugin_link{"static"};
#else
    const std::string plugin_link{"dynamic"};
#endif
    LogPrintf("Qt %s (%s), plugin=%s (%s)\n", qVersion(), qt_link, QGuiApplication::platformName().toStdString(), plugin_link);
    const auto static_plugins = QPluginLoader::staticPlugins();
    if (static_plugins.empty()) {
        LogPrintf("No static plugins.\n");
    } else {
        LogPrintf("Static plugins:\n");
        for (const QStaticPlugin& p : static_plugins) {
            QJsonObject meta_data = p.metaData();
            const std::string plugin_class = meta_data.take(QString("className")).toString().toStdString();
            const int plugin_version = meta_data.take(QString("version")).toInt();
            LogPrintf(" %s, version %d\n", plugin_class, plugin_version);
        }
    }
    LogPrintf("Style: %s / %s\n", QApplication::style()->objectName().toStdString(), QApplication::style()->metaObject()->className());
    LogPrintf("System: %s, %s\n", QSysInfo::prettyProductName().toStdString(), QSysInfo::buildAbi().toStdString());
    for (const QScreen* s : QGuiApplication::screens()) {
        LogPrintf("Screen: %s %dx%d, pixel ratio=%.1f\n", s->name().toStdString(), s->size().width(), s->size().height(), s->devicePixelRatio());
    }
}
void PopupMenu(QMenu* menu, const QPoint& point, QAction* at_action)
{
    if (QApplication::platformName() == "minimal") return;
    menu->popup(point, at_action);
}
QDateTime StartOfDay(const QDate& date)
{
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
    return date.startOfDay();
#else
    return QDateTime(date);
#endif
}
bool HasPixmap(const QLabel* label)
{
#if (QT_VERSION >= QT_VERSION_CHECK(5, 15, 0))
    return !label->pixmap(Qt::ReturnByValue).isNull();
#else
    return label->pixmap() != nullptr;
#endif
}
QImage GetImage(const QLabel* label)
{
    if (!HasPixmap(label)) {
        return QImage();
    }
#if (QT_VERSION >= QT_VERSION_CHECK(5, 15, 0))
    return label->pixmap(Qt::ReturnByValue).toImage();
#else
    return label->pixmap()->toImage();
#endif
}
QString MakeHtmlLink(const QString& source, const QString& link)
{
    return QString(source).replace(
        link,
        QLatin1String("<a href=\"") + link + QLatin1String("\">") + link + QLatin1String("</a>"));
}
void PrintSlotException(
    const std::exception* exception,
    const QObject* sender,
    const QObject* receiver)
{
    std::string description = sender->metaObject()->className();
    description += "->";
    description += receiver->metaObject()->className();
    PrintExceptionContinue(exception, description);
}
void ShowModalDialogAsynchronously(QDialog* dialog)
{
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setWindowModality(Qt::ApplicationModal);
    dialog->show();
}
}
