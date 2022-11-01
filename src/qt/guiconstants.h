#ifndef LCRYP_QT_GUICONSTANTS_H
#define LCRYP_QT_GUICONSTANTS_H
#include <chrono>
#include <cstdint>
using namespace std::chrono_literals;
static constexpr auto MODEL_UPDATE_DELAY{250ms};
static constexpr auto SHUTDOWN_POLLING_DELAY{200ms};
static const int MAX_PASSPHRASE_SIZE = 1024;
static const int STATUSBAR_ICONSIZE = 16;
static const bool DEFAULT_SPLASHSCREEN = true;
#define STYLE_INVALID "background:#FF8080"
#define COLOR_UNCONFIRMED QColor(128, 128, 128)
#define COLOR_NEGATIVE QColor(255, 0, 0)
#define COLOR_BAREADDRESS QColor(140, 140, 140)
#define COLOR_TX_STATUS_DANGER QColor(200, 100, 100)
#define COLOR_BLACK QColor(0, 0, 0)
static const int TOOLTIP_WRAP_THRESHOLD = 80;
#define SPINNER_FRAMES 36
#define QAPP_ORG_NAME "LcRyp"
#define QAPP_ORG_DOMAIN "lcryp.org"
#define QAPP_APP_NAME_DEFAULT "LcRyp-Qt"
#define QAPP_APP_NAME_TESTNET "LcRyp-Qt-testnet"
#define QAPP_APP_NAME_SIGNET "LcRyp-Qt-signet"
#define QAPP_APP_NAME_REGTEST "LcRyp-Qt-regtest"
static constexpr uint64_t GB_BYTES{1073741824};
static constexpr int DEFAULT_PRUNE_TARGET_GB{1};
#endif
