#include <locale.h>
#include <libintl.h>
#define i18n(s) gettext(s)
#include <iostream>
using namespace std;
int main()
{
    bindtextdomain("main", "messages");
    textdomain("main");
    setlocale(LC_MESSAGES, "ru_RU.KOI8-R");
    std::cout << i18n("hello") << "\n";
    return 0;
}
