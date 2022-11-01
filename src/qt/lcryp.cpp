#if defined(HAVE_CONFIG_H)
#include <config/lcryp-config.h>
#endif
#include <qt/lcryp.h>
#include <chainparams.h>
#include <init.h>
#include <interfaces/handler.h>
#include <interfaces/init.h>
#include <interfaces/node.h>
#include <node/interface_ui.h>
#include <noui.h>
#include <qt/lcrypgui.h>
#include <qt/clientmodel.h>
#include <qt/guiconstants.h>
#include <qt/guiutil.h>
#include <qt/initexecutor.h>
#include <qt/intro.h>
#include <qt/networkstyle.h>
#include <qt/optionsmodel.h>
#include <qt/platformstyle.h>
#include <qt/splashscreen.h>
#include <qt/utilitydialog.h>
#include <qt/winshutdownmonitor.h>
#include <uint256.h>
#include <util/string.h>
#include <util/system.h>
#include <util/threadnames.h>
#include <util/translation.h>
#include <validation.h>
#ifdef ENABLE_WALLET
#include <qt/paymentserver.h>
#include <qt/walletcontroller.h>
#include <qt/walletmodel.h>
#endif
#include <boost/signals2/connection.hpp>
#include <chrono>
#include <memory>
#include <QApplication>
#include <QDebug>
#include <QLatin1String>
#include <QLibraryInfo>
#include <QLocale>
#include <QMessageBox>
#include <QSettings>
#include <QThread>
#include <QTimer>
#include <QTranslator>
#include <QWindow>
#if defined(QT_STATICPLUGIN)
#include <QtPlugin>
#if defined(QT_QPA_PLATFORM_XCB)
Q_IMPORT_PLUGIN(QXcbIntegrationPlugin);
#elif defined(QT_QPA_PLATFORM_WINDOWS)
Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin);
Q_IMPORT_PLUGIN(QWindowsVistaStylePlugin);
#elif defined(QT_QPA_PLATFORM_COCOA)
Q_IMPORT_PLUGIN(QCocoaIntegrationPlugin);
Q_IMPORT_PLUGIN(QMacStylePlugin);
#elif defined(QT_QPA_PLATFORM_ANDROID)
Q_IMPORT_PLUGIN(QAndroidPlatformIntegrationPlugin)
#endif
#endif
Q_DECLARE_METATYPE(bool*)
Q_DECLARE_METATYPE(CAmount)
Q_DECLARE_METATYPE(SynchronizationState)
Q_DECLARE_METATYPE(SyncType)
Q_DECLARE_METATYPE(uint256)
static void RegisterMetaTypes()
{
    qRegisterMetaType<bool*>();
    qRegisterMetaType<SynchronizationState>();
    qRegisterMetaType<SyncType>();
  #ifdef ENABLE_WALLET
    qRegisterMetaType<WalletModel*>();
  #endif
    qRegisterMetaType<CAmount>("CAmount");
    qRegisterMetaType<size_t>("size_t");
    qRegisterMetaType<std::function<void()>>("std::function<void()>");
    qRegisterMetaType<QMessageBox::Icon>("QMessageBox::Icon");
    qRegisterMetaType<interfaces::BlockAndHeaderTipInfo>("interfaces::BlockAndHeaderTipInfo");
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    qRegisterMetaTypeStreamOperators<LcRypUnit>("LcRypUnit");
#else
    qRegisterMetaType<LcRypUnit>("LcRypUnit");
#endif
}
static QString GetLangTerritory()
{
    QSettings settings;
    QString lang_territory = QLocale::system().name();
    QString lang_territory_qsettings = settings.value("language", "").toString();
    if(!lang_territory_qsettings.isEmpty())
        lang_territory = lang_territory_qsettings;
    lang_territory = QString::fromStdString(gArgs.GetArg("-lang", lang_territory.toStdString()));
    return lang_territory;
}
static void initTranslations(QTranslator &qtTranslatorBase, QTranslator &qtTranslator, QTranslator &translatorBase, QTranslator &translator)
{
    QApplication::removeTranslator(&qtTranslatorBase);
    QApplication::removeTranslator(&qtTranslator);
    QApplication::removeTranslator(&translatorBase);
    QApplication::removeTranslator(&translator);
    QString lang_territory = GetLangTerritory();
    QString lang = lang_territory;
    lang.truncate(lang_territory.lastIndexOf('_'));
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    const QString translation_path{QLibraryInfo::location(QLibraryInfo::TranslationsPath)};
#else
    const QString translation_path{QLibraryInfo::path(QLibraryInfo::TranslationsPath)};
#endif
    if (qtTranslatorBase.load("qt_" + lang, translation_path)) {
        QApplication::installTranslator(&qtTranslatorBase);
    }
    if (qtTranslator.load("qt_" + lang_territory, translation_path)) {
        QApplication::installTranslator(&qtTranslator);
    }
    if (translatorBase.load(lang, ":/translations/")) {
        QApplication::installTranslator(&translatorBase);
    }
    if (translator.load(lang_territory, ":/translations/")) {
        QApplication::installTranslator(&translator);
    }
}
static bool InitSettings()
{
    if (!gArgs.GetSettingsPath()) {
        return true;
    }
    std::vector<std::string> errors;
    if (!gArgs.ReadSettingsFile(&errors)) {
        std::string error = QT_TRANSLATE_NOOP("lcryp-core", "Settings file could not be read");
        std::string error_translated = QCoreApplication::translate("lcryp-core", error.c_str()).toStdString();
        InitError(Untranslated(strprintf("%s:\n%s\n", error, MakeUnorderedList(errors))));
        QMessageBox messagebox(QMessageBox::Critical, PACKAGE_NAME, QString::fromStdString(strprintf("%s.", error_translated)), QMessageBox::Reset | QMessageBox::Abort);
        messagebox.setInformativeText(QObject::tr("Do you want to reset settings to default values, or to abort without making changes?"));
        messagebox.setDetailedText(QString::fromStdString(MakeUnorderedList(errors)));
        messagebox.setTextFormat(Qt::PlainText);
        messagebox.setDefaultButton(QMessageBox::Reset);
        switch (messagebox.exec()) {
        case QMessageBox::Reset:
            break;
        case QMessageBox::Abort:
            return false;
        default:
            assert(false);
        }
    }
    errors.clear();
    if (!gArgs.WriteSettingsFile(&errors)) {
        std::string error = QT_TRANSLATE_NOOP("lcryp-core", "Settings file could not be written");
        std::string error_translated = QCoreApplication::translate("lcryp-core", error.c_str()).toStdString();
        InitError(Untranslated(strprintf("%s:\n%s\n", error, MakeUnorderedList(errors))));
        QMessageBox messagebox(QMessageBox::Critical, PACKAGE_NAME, QString::fromStdString(strprintf("%s.", error_translated)), QMessageBox::Ok);
        messagebox.setInformativeText(QObject::tr("A fatal error occurred. Check that settings file is writable, or try running with -nosettings."));
        messagebox.setDetailedText(QString::fromStdString(MakeUnorderedList(errors)));
        messagebox.setTextFormat(Qt::PlainText);
        messagebox.setDefaultButton(QMessageBox::Ok);
        messagebox.exec();
        return false;
    }
    return true;
}
void DebugMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString &msg)
{
    Q_UNUSED(context);
    if (type == QtDebugMsg) {
        LogPrint(BCLog::QT, "GUI: %s\n", msg.toStdString());
    } else {
        LogPrintf("GUI: %s\n", msg.toStdString());
    }
}
static int qt_argc = 1;
static const char* qt_argv = "lcryp-qt";
LcRypApplication::LcRypApplication():
    QApplication(qt_argc, const_cast<char **>(&qt_argv)),
    optionsModel(nullptr),
    clientModel(nullptr),
    window(nullptr),
    pollShutdownTimer(nullptr),
    returnValue(0),
    platformStyle(nullptr)
{
    RegisterMetaTypes();
    setQuitOnLastWindowClosed(false);
}
void LcRypApplication::setupPlatformStyle()
{
    std::string platformName;
    platformName = gArgs.GetArg("-uiplatform", LcRypGUI::DEFAULT_UIPLATFORM);
    platformStyle = PlatformStyle::instantiate(QString::fromStdString(platformName));
    if (!platformStyle)
        platformStyle = PlatformStyle::instantiate("other");
    assert(platformStyle);
}
LcRypApplication::~LcRypApplication()
{
    m_executor.reset();
    delete window;
    window = nullptr;
    delete platformStyle;
    platformStyle = nullptr;
}
#ifdef ENABLE_WALLET
void LcRypApplication::createPaymentServer()
{
    paymentServer = new PaymentServer(this);
}
#endif
bool LcRypApplication::createOptionsModel(bool resetSettings)
{
    optionsModel = new OptionsModel(node(), this);
    if (resetSettings) {
        optionsModel->Reset();
    }
    bilingual_str error;
    if (!optionsModel->Init(error)) {
        fs::path settings_path;
        if (gArgs.GetSettingsPath(&settings_path)) {
            error += Untranslated("\n");
            std::string quoted_path = strprintf("%s", fs::quoted(fs::PathToString(settings_path)));
            error.original += strprintf("Settings file %s might be corrupt or invalid.", quoted_path);
            error.translated += tr("Settings file %1 might be corrupt or invalid.").arg(QString::fromStdString(quoted_path)).toStdString();
        }
        InitError(error);
        QMessageBox::critical(nullptr, PACKAGE_NAME, QString::fromStdString(error.translated));
        return false;
    }
    return true;
}
void LcRypApplication::createWindow(const NetworkStyle *networkStyle)
{
    window = new LcRypGUI(node(), platformStyle, networkStyle, nullptr);
    connect(window, &LcRypGUI::quitRequested, this, &LcRypApplication::requestShutdown);
    pollShutdownTimer = new QTimer(window);
    connect(pollShutdownTimer, &QTimer::timeout, [this]{
        if (!QApplication::activeModalWidget()) {
            window->detectShutdown();
        }
    });
}
void LcRypApplication::createSplashScreen(const NetworkStyle *networkStyle)
{
    assert(!m_splash);
    m_splash = new SplashScreen(networkStyle);
    m_splash->show();
    connect(this, &LcRypApplication::splashFinished, m_splash, &SplashScreen::finish);
    connect(this, &LcRypApplication::requestedShutdown, m_splash, &QWidget::close);
}
void LcRypApplication::createNode(interfaces::Init& init)
{
    assert(!m_node);
    m_node = init.makeNode();
    if (m_splash) m_splash->setNode(*m_node);
}
bool LcRypApplication::baseInitialize()
{
    return node().baseInitialize();
}
void LcRypApplication::startThread()
{
    assert(!m_executor);
    m_executor.emplace(node());
    connect(&m_executor.value(), &InitExecutor::initializeResult, this, &LcRypApplication::initializeResult);
    connect(&m_executor.value(), &InitExecutor::shutdownResult, this, [] {
        QCoreApplication::exit(0);
    });
    connect(&m_executor.value(), &InitExecutor::runawayException, this, &LcRypApplication::handleRunawayException);
    connect(this, &LcRypApplication::requestedInitialize, &m_executor.value(), &InitExecutor::initialize);
    connect(this, &LcRypApplication::requestedShutdown, &m_executor.value(), &InitExecutor::shutdown);
}
void LcRypApplication::parameterSetup()
{
    gArgs.SoftSetBoolArg("-printtoconsole", false);
    InitLogging(gArgs);
    InitParameterInteraction(gArgs);
}
void LcRypApplication::InitPruneSetting(int64_t prune_MiB)
{
    optionsModel->SetPruneTargetGB(PruneMiBtoGB(prune_MiB));
}
void LcRypApplication::requestInitialize()
{
    qDebug() << __func__ << ": Requesting initialize";
    startThread();
    Q_EMIT requestedInitialize();
}
void LcRypApplication::requestShutdown()
{
    for (const auto w : QGuiApplication::topLevelWindows()) {
        w->hide();
    }
    shutdownWindow.reset(ShutdownWindow::showShutdownWindow(window));
    qDebug() << __func__ << ": Requesting shutdown";
    window->unsubscribeFromCoreSignals();
    node().startShutdown();
    window->setClientModel(nullptr);
    pollShutdownTimer->stop();
#ifdef ENABLE_WALLET
    delete m_wallet_controller;
    m_wallet_controller = nullptr;
#endif
    delete clientModel;
    clientModel = nullptr;
    Q_EMIT requestedShutdown();
}
void LcRypApplication::initializeResult(bool success, interfaces::BlockAndHeaderTipInfo tip_info)
{
    qDebug() << __func__ << ": Initialization result: " << success;
    returnValue = success ? EXIT_SUCCESS : EXIT_FAILURE;
    if(success)
    {
        qInfo() << "Platform customization:" << platformStyle->getName();
        clientModel = new ClientModel(node(), optionsModel);
        window->setClientModel(clientModel, &tip_info);
#ifdef ENABLE_WALLET
        if (WalletModel::isWalletEnabled()) {
            m_wallet_controller = new WalletController(*clientModel, platformStyle, this);
            window->setWalletController(m_wallet_controller);
            if (paymentServer) {
                paymentServer->setOptionsModel(optionsModel);
            }
        }
#endif
        if (!gArgs.GetBoolArg("-min", false)) {
            window->show();
        } else if (clientModel->getOptionsModel()->getMinimizeToTray() && window->hasTrayIcon()) {
        } else {
            window->showMinimized();
        }
        Q_EMIT splashFinished();
        Q_EMIT windowShown(window);
#ifdef ENABLE_WALLET
        if (paymentServer) {
            connect(paymentServer, &PaymentServer::receivedPaymentRequest, window, &LcRypGUI::handlePaymentRequest);
            connect(window, &LcRypGUI::receivedURI, paymentServer, &PaymentServer::handleURIOrFile);
            connect(paymentServer, &PaymentServer::message, [this](const QString& title, const QString& message, unsigned int style) {
                window->message(title, message, style);
            });
            QTimer::singleShot(100ms, paymentServer, &PaymentServer::uiReady);
        }
#endif
        pollShutdownTimer->start(SHUTDOWN_POLLING_DELAY);
    } else {
        Q_EMIT splashFinished();
        requestShutdown();
    }
}
void LcRypApplication::handleRunawayException(const QString &message)
{
    QMessageBox::critical(
        nullptr, tr("Runaway exception"),
        tr("A fatal error occurred. %1 can no longer continue safely and will quit.").arg(PACKAGE_NAME) +
        QLatin1String("<br><br>") + GUIUtil::MakeHtmlLink(message, PACKAGE_BUGREPORT));
    ::exit(EXIT_FAILURE);
}
void LcRypApplication::handleNonFatalException(const QString& message)
{
    assert(QThread::currentThread() == thread());
    QMessageBox::warning(
        nullptr, tr("Internal error"),
        tr("An internal error occurred. %1 will attempt to continue safely. This is "
           "an unexpected bug which can be reported as described below.").arg(PACKAGE_NAME) +
        QLatin1String("<br><br>") + GUIUtil::MakeHtmlLink(message, PACKAGE_BUGREPORT));
}
WId LcRypApplication::getMainWinId() const
{
    if (!window)
        return 0;
    return window->winId();
}
bool LcRypApplication::event(QEvent* e)
{
    if (e->type() == QEvent::Quit) {
        requestShutdown();
        return true;
    }
    return QApplication::event(e);
}
static void SetupUIArgs(ArgsManager& argsman)
{
    argsman.AddArg("-choosedatadir", strprintf("Choose data directory on startup (default: %u)", DEFAULT_CHOOSE_DATADIR), ArgsManager::ALLOW_ANY, OptionsCategory::GUI);
    argsman.AddArg("-lang=<lang>", "Set language, for example \"de_DE\" (default: system locale)", ArgsManager::ALLOW_ANY, OptionsCategory::GUI);
    argsman.AddArg("-min", "Start minimized", ArgsManager::ALLOW_ANY, OptionsCategory::GUI);
    argsman.AddArg("-resetguisettings", "Reset all settings changed in the GUI", ArgsManager::ALLOW_ANY, OptionsCategory::GUI);
    argsman.AddArg("-splash", strprintf("Show splash screen on startup (default: %u)", DEFAULT_SPLASHSCREEN), ArgsManager::ALLOW_ANY, OptionsCategory::GUI);
    argsman.AddArg("-uiplatform", strprintf("Select platform to customize UI for (one of windows, macosx, other; default: %s)", LcRypGUI::DEFAULT_UIPLATFORM), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::GUI);
}
int GuiMain(int argc, char* argv[])
{
#ifdef WIN32
    util::WinCmdLineArgs winArgs;
    std::tie(argc, argv) = winArgs.get();
#endif
    std::unique_ptr<interfaces::Init> init = interfaces::MakeGuiInit(argc, argv);
    SetupEnvironment();
    util::ThreadSetInternalName("main");
    boost::signals2::scoped_connection handler_message_box = ::uiInterface.ThreadSafeMessageBox_connect(noui_ThreadSafeMessageBox);
    boost::signals2::scoped_connection handler_question = ::uiInterface.ThreadSafeQuestion_connect(noui_ThreadSafeQuestion);
    boost::signals2::scoped_connection handler_init_message = ::uiInterface.InitMessage_connect(noui_InitMessage);
    Q_INIT_RESOURCE(lcryp);
    Q_INIT_RESOURCE(lcryp_locale);
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
#if defined(QT_QPA_PLATFORM_ANDROID)
    QApplication::setAttribute(Qt::AA_DontUseNativeMenuBar);
    QApplication::setAttribute(Qt::AA_DontCreateNativeWidgetSiblings);
    QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
#endif
    LcRypApplication app;
    GUIUtil::LoadFont(QStringLiteral(":/fonts/monospace"));
    SetupServerArgs(gArgs);
    SetupUIArgs(gArgs);
    std::string error;
    if (!gArgs.ParseParameters(argc, argv, error)) {
        InitError(strprintf(Untranslated("Error parsing command line arguments: %s\n"), error));
        QMessageBox::critical(nullptr, PACKAGE_NAME,
            QString::fromStdString("Error parsing command line arguments: %1.").arg(QString::fromStdString(error)));
        return EXIT_FAILURE;
    }
    app.setupPlatformStyle();
    QApplication::setOrganizationName(QAPP_ORG_NAME);
    QApplication::setOrganizationDomain(QAPP_ORG_DOMAIN);
    QApplication::setApplicationName(QAPP_APP_NAME_DEFAULT);
    QTranslator qtTranslatorBase, qtTranslator, translatorBase, translator;
    initTranslations(qtTranslatorBase, qtTranslator, translatorBase, translator);
    if (HelpRequested(gArgs) || gArgs.IsArgSet("-version")) {
        HelpMessageDialog help(nullptr, gArgs.IsArgSet("-version"));
        help.showOrPrint();
        return EXIT_SUCCESS;
    }
    app.installEventFilter(new GUIUtil::ToolTipToRichTextFilter(TOOLTIP_WRAP_THRESHOLD, &app));
    bool did_show_intro = false;
    int64_t prune_MiB = 0;
    if (!Intro::showIfNeeded(did_show_intro, prune_MiB)) return EXIT_SUCCESS;
    if (!CheckDataDirOption()) {
        InitError(strprintf(Untranslated("Specified data directory \"%s\" does not exist.\n"), gArgs.GetArg("-datadir", "")));
        QMessageBox::critical(nullptr, PACKAGE_NAME,
            QObject::tr("Error: Specified data directory \"%1\" does not exist.").arg(QString::fromStdString(gArgs.GetArg("-datadir", ""))));
        return EXIT_FAILURE;
    }
    if (!gArgs.ReadConfigFiles(error, true)) {
        InitError(strprintf(Untranslated("Error reading configuration file: %s\n"), error));
        QMessageBox::critical(nullptr, PACKAGE_NAME,
            QObject::tr("Error: Cannot parse configuration file: %1.").arg(QString::fromStdString(error)));
        return EXIT_FAILURE;
    }
    try {
        SelectParams(gArgs.GetChainName());
    } catch(std::exception &e) {
        InitError(Untranslated(strprintf("%s\n", e.what())));
        QMessageBox::critical(nullptr, PACKAGE_NAME, QObject::tr("Error: %1").arg(e.what()));
        return EXIT_FAILURE;
    }
#ifdef ENABLE_WALLET
    PaymentServer::ipcParseCommandLine(argc, argv);
#endif
    if (!InitSettings()) {
        return EXIT_FAILURE;
    }
    QScopedPointer<const NetworkStyle> networkStyle(NetworkStyle::instantiate(Params().NetworkIDString()));
    assert(!networkStyle.isNull());
    QApplication::setApplicationName(networkStyle->getAppName());
    initTranslations(qtTranslatorBase, qtTranslator, translatorBase, translator);
#ifdef ENABLE_WALLET
    if (PaymentServer::ipcSendCommandLine())
        exit(EXIT_SUCCESS);
    if (WalletModel::isWalletEnabled()) {
        app.createPaymentServer();
    }
#endif
    app.installEventFilter(new GUIUtil::LabelOutOfFocusEventFilter(&app));
#if defined(Q_OS_WIN)
    qApp->installNativeEventFilter(new WinShutdownMonitor());
#endif
    qInstallMessageHandler(DebugMessageHandler);
    app.parameterSetup();
    GUIUtil::LogQtInfo();
    if (gArgs.GetBoolArg("-splash", DEFAULT_SPLASHSCREEN) && !gArgs.GetBoolArg("-min", false))
        app.createSplashScreen(networkStyle.data());
    app.createNode(*init);
    if (!app.createOptionsModel(gArgs.GetBoolArg("-resetguisettings", false))) {
        return EXIT_FAILURE;
    }
    if (did_show_intro) {
        app.InitPruneSetting(prune_MiB);
    }
    int rv = EXIT_SUCCESS;
    try
    {
        app.createWindow(networkStyle.data());
        if (app.baseInitialize()) {
            app.requestInitialize();
#if defined(Q_OS_WIN)
            WinShutdownMonitor::registerShutdownBlockReason(QObject::tr("%1 didn't yet exit safely…").arg(PACKAGE_NAME), (HWND)app.getMainWinId());
#endif
            app.exec();
            rv = app.getReturnValue();
        } else {
            rv = EXIT_FAILURE;
        }
    } catch (const std::exception& e) {
        PrintExceptionContinue(&e, "Runaway exception");
        app.handleRunawayException(QString::fromStdString(app.node().getWarnings().translated));
    } catch (...) {
        PrintExceptionContinue(nullptr, "Runaway exception");
        app.handleRunawayException(QString::fromStdString(app.node().getWarnings().translated));
    }
    return rv;
}
