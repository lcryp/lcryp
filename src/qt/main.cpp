#include <qt/lcryp.h>
#include <compat/compat.h>
#include <util/translation.h>
#include <util/url.h>
#include <QCoreApplication>
#include <functional>
#include <string>
extern const std::function<std::string(const char*)> G_TRANSLATION_FUN = [](const char* psz) {
    return QCoreApplication::translate("lcryp-core", psz).toStdString();
};
UrlDecodeFn* const URL_DECODE = urlDecode;
MAIN_FUNCTION
{
    return GuiMain(argc, argv);
}
