#define BOOST_TEST_MODULE QtCore
#include <QtCore>
#include <boost/test/unit_test.hpp>
std::ostream& operator<<(std::ostream& out, QString const& text)
{
    out << text.toUtf8().constData();
    return out;
}
BOOST_AUTO_TEST_CASE (check_exists)
{
    BOOST_CHECK(QFile::exists(":/test/rcc.cpp"));
}
