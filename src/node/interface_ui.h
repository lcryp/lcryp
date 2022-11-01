#ifndef LCRYP_NODE_INTERFACE_UI_H
#define LCRYP_NODE_INTERFACE_UI_H
#include <functional>
#include <memory>
#include <string>
class CBlockIndex;
enum class SynchronizationState;
struct bilingual_str;
namespace boost {
namespace signals2 {
class connection;
}
}
class CClientUIInterface
{
public:
    enum MessageBoxFlags : uint32_t {
        ICON_INFORMATION    = 0,
        ICON_WARNING        = (1U << 0),
        ICON_ERROR          = (1U << 1),
        ICON_MASK = (ICON_INFORMATION | ICON_WARNING | ICON_ERROR),
        BTN_OK      = 0x00000400U,
        BTN_YES     = 0x00004000U,
        BTN_NO      = 0x00010000U,
        BTN_ABORT   = 0x00040000U,
        BTN_RETRY   = 0x00080000U,
        BTN_IGNORE  = 0x00100000U,
        BTN_CLOSE   = 0x00200000U,
        BTN_CANCEL  = 0x00400000U,
        BTN_DISCARD = 0x00800000U,
        BTN_HELP    = 0x01000000U,
        BTN_APPLY   = 0x02000000U,
        BTN_RESET   = 0x04000000U,
        BTN_MASK = (BTN_OK | BTN_YES | BTN_NO | BTN_ABORT | BTN_RETRY | BTN_IGNORE |
                    BTN_CLOSE | BTN_CANCEL | BTN_DISCARD | BTN_HELP | BTN_APPLY | BTN_RESET),
        MODAL               = 0x10000000U,
        SECURE              = 0x40000000U,
        MSG_INFORMATION = ICON_INFORMATION,
        MSG_WARNING = (ICON_WARNING | BTN_OK | MODAL),
        MSG_ERROR = (ICON_ERROR | BTN_OK | MODAL)
    };
#define ADD_SIGNALS_DECL_WRAPPER(signal_name, rtype, ...)                                  \
    rtype signal_name(__VA_ARGS__);                                                        \
    using signal_name##Sig = rtype(__VA_ARGS__);                                           \
    boost::signals2::connection signal_name##_connect(std::function<signal_name##Sig> fn);
    ADD_SIGNALS_DECL_WRAPPER(ThreadSafeMessageBox, bool, const bilingual_str& message, const std::string& caption, unsigned int style);
    ADD_SIGNALS_DECL_WRAPPER(ThreadSafeQuestion, bool, const bilingual_str& message, const std::string& noninteractive_message, const std::string& caption, unsigned int style);
    ADD_SIGNALS_DECL_WRAPPER(InitMessage, void, const std::string& message);
    ADD_SIGNALS_DECL_WRAPPER(InitWallet, void, );
    ADD_SIGNALS_DECL_WRAPPER(NotifyNumConnectionsChanged, void, int newNumConnections);
    ADD_SIGNALS_DECL_WRAPPER(NotifyNetworkActiveChanged, void, bool networkActive);
    ADD_SIGNALS_DECL_WRAPPER(NotifyAlertChanged, void, );
    ADD_SIGNALS_DECL_WRAPPER(ShowProgress, void, const std::string& title, int nProgress, bool resume_possible);
    ADD_SIGNALS_DECL_WRAPPER(NotifyBlockTip, void, SynchronizationState, const CBlockIndex*);
    ADD_SIGNALS_DECL_WRAPPER(NotifyHeaderTip, void, SynchronizationState, int64_t height, int64_t timestamp, bool presync);
    ADD_SIGNALS_DECL_WRAPPER(BannedListChanged, void, void);
};
void InitWarning(const bilingual_str& str);
bool InitError(const bilingual_str& str);
constexpr auto AbortError = InitError;
extern CClientUIInterface uiInterface;
#endif
