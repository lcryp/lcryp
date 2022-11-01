#if defined(HAVE_CONFIG_H)
#include <config/lcryp-config.h>
#endif
#include <chainparams.h>
#include <clientversion.h>
#include <compat/compat.h>
#include <init.h>
#include <interfaces/chain.h>
#include <interfaces/init.h>
#include <node/context.h>
#include <node/interface_ui.h>
#include <noui.h>
#include <shutdown.h>
#include <util/check.h>
#include <util/strencodings.h>
#include <util/syscall_sandbox.h>
#include <util/syserror.h>
#include <util/system.h>
#include <util/threadnames.h>
#include <util/tokenpipe.h>
#include <util/translation.h>
#include <util/url.h>
#include <any>
#include <functional>
#include <optional>
using node::NodeContext;
const std::function<std::string(const char*)> G_TRANSLATION_FUN = nullptr;
UrlDecodeFn* const URL_DECODE = urlDecode;
#if HAVE_DECL_FORK
int fork_daemon(bool nochdir, bool noclose, TokenPipeEnd& endpoint)
{
    std::optional<TokenPipe> umbilical = TokenPipe::Make();
    if (!umbilical) {
        return -1;
    }
    int pid = fork();
    if (pid < 0) {
        return -1;
    }
    if (pid != 0) {
        endpoint = umbilical->TakeReadEnd();
        umbilical->TakeWriteEnd().Close();
        int status = endpoint.TokenRead();
        if (status != 0) {
            endpoint.Close();
            return -1;
        }
        return pid;
    }
    endpoint = umbilical->TakeWriteEnd();
    umbilical->TakeReadEnd().Close();
#if HAVE_DECL_SETSID
    if (setsid() < 0) {
        exit(1);
    }
#endif
    if (!nochdir) {
        if (chdir("/") != 0) {
            exit(1);
        }
    }
    if (!noclose) {
        int fd = open("/dev/null", O_RDWR);
        if (fd >= 0) {
            bool err = dup2(fd, STDIN_FILENO) < 0 || dup2(fd, STDOUT_FILENO) < 0 || dup2(fd, STDERR_FILENO) < 0;
            if (fd > 2) close(fd);
            if (err) {
                exit(1);
            }
        } else {
            exit(1);
        }
    }
    endpoint.TokenWrite(0);
    return 0;
}
#endif
static bool AppInit(NodeContext& node, int argc, char* argv[])
{
    bool fRet = false;
    util::ThreadSetInternalName("init");
    ArgsManager& args = *Assert(node.args);
    SetupServerArgs(args);
    std::string error;
    if (!args.ParseParameters(argc, argv, error)) {
        return InitError(Untranslated(strprintf("Error parsing command line arguments: %s\n", error)));
    }
    if (HelpRequested(args) || args.IsArgSet("-version")) {
        std::string strUsage = PACKAGE_NAME " version " + FormatFullVersion() + "\n";
        if (args.IsArgSet("-version")) {
            strUsage += FormatParagraph(LicenseInfo());
        } else {
            strUsage += "\nUsage:  lcrypd [options]                     Start " PACKAGE_NAME "\n"
                "\n";
            strUsage += args.GetHelpMessage();
        }
        tfm::format(std::cout, "%s", strUsage);
        return true;
    }
#if HAVE_DECL_FORK
    TokenPipeEnd daemon_ep;
#endif
    std::any context{&node};
    try
    {
        if (!CheckDataDirOption()) {
            return InitError(Untranslated(strprintf("Specified data directory \"%s\" does not exist.\n", args.GetArg("-datadir", ""))));
        }
        if (!args.ReadConfigFiles(error, true)) {
            return InitError(Untranslated(strprintf("Error reading configuration file: %s\n", error)));
        }
        try {
            SelectParams(args.GetChainName());
        } catch (const std::exception& e) {
            return InitError(Untranslated(strprintf("%s\n", e.what())));
        }
        for (int i = 1; i < argc; i++) {
            if (!IsSwitchChar(argv[i][0])) {
                return InitError(Untranslated(strprintf("Command line contains unexpected token '%s', see lcrypd -h for a list of options.\n", argv[i])));
            }
        }
        if (!args.InitSettings(error)) {
            InitError(Untranslated(error));
            return false;
        }
        args.SoftSetBoolArg("-server", true);
        InitLogging(args);
        InitParameterInteraction(args);
        if (!AppInitBasicSetup(args)) {
            return false;
        }
        if (!AppInitParameterInteraction(args)) {
            return false;
        }
        node.kernel = std::make_unique<kernel::Context>();
        if (!AppInitSanityChecks(*node.kernel))
        {
            return false;
        }
        if (args.GetBoolArg("-daemon", DEFAULT_DAEMON) || args.GetBoolArg("-daemonwait", DEFAULT_DAEMONWAIT)) {
#if HAVE_DECL_FORK
            tfm::format(std::cout, PACKAGE_NAME " starting\n");
            switch (fork_daemon(1, 0, daemon_ep)) {
            case 0:
                if (!args.GetBoolArg("-daemonwait", DEFAULT_DAEMONWAIT)) {
                    daemon_ep.TokenWrite(1);
                    daemon_ep.Close();
                }
                break;
            case -1:
                return InitError(Untranslated(strprintf("fork_daemon() failed: %s\n", SysErrorString(errno))));
            default: {
                int token = daemon_ep.TokenRead();
                if (token) {
                    exit(EXIT_SUCCESS);
                } else {
                    tfm::format(std::cerr, "Error during initialization - check debug.log for details\n");
                    exit(EXIT_FAILURE);
                }
            }
            }
#else
            return InitError(Untranslated("-daemon is not supported on this operating system\n"));
#endif
        }
        if (!AppInitLockDataDirectory())
        {
            return false;
        }
        fRet = AppInitInterfaces(node) && AppInitMain(node);
    }
    catch (const std::exception& e) {
        PrintExceptionContinue(&e, "AppInit()");
    } catch (...) {
        PrintExceptionContinue(nullptr, "AppInit()");
    }
#if HAVE_DECL_FORK
    if (daemon_ep.IsOpen()) {
        daemon_ep.TokenWrite(fRet);
        daemon_ep.Close();
    }
#endif
    SetSyscallSandboxPolicy(SyscallSandboxPolicy::SHUTOFF);
    if (fRet) {
        WaitForShutdown();
    }
    Interrupt(node);
    Shutdown(node);
    return fRet;
}
MAIN_FUNCTION
{
#ifdef WIN32
    util::WinCmdLineArgs winArgs;
    std::tie(argc, argv) = winArgs.get();
#endif
    NodeContext node;
    int exit_status;
    std::unique_ptr<interfaces::Init> init = interfaces::MakeNodeInit(node, argc, argv, exit_status);
    if (!init) {
        return exit_status;
    }
    SetupEnvironment();
    noui_connect();
    return (AppInit(node, argc, argv) ? EXIT_SUCCESS : EXIT_FAILURE);
}
