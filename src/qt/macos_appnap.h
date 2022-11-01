#ifndef LCRYP_QT_MACOS_APPNAP_H
#define LCRYP_QT_MACOS_APPNAP_H
#include <memory>
class CAppNapInhibitor final
{
public:
    explicit CAppNapInhibitor();
    ~CAppNapInhibitor();
    void disableAppNap();
    void enableAppNap();
private:
    class CAppNapImpl;
    std::unique_ptr<CAppNapImpl> impl;
};
#endif
