#define BOOST_TEST_MODULE QtNetwork
#include <QHostInfo>
#include <QTextStream>
#include <boost/test/unit_test.hpp>
#include <iostream>
BOOST_AUTO_TEST_CASE (defines)
{
    BOOST_CHECK_EQUAL(BOOST_IS_DEFINED(QT_CORE_LIB), true);
    BOOST_CHECK_EQUAL(BOOST_IS_DEFINED(QT_NETWORK_LIB), true);
}
BOOST_AUTO_TEST_CASE( hostname )
{
    QHostInfo info(QHostInfo::fromName("www.boost.org"));
    QTextStream stream(stdout, QIODevice::WriteOnly);
    Q_FOREACH(QHostAddress address, info.addresses())
    {
        BOOST_CHECK_EQUAL(address.isNull(), false);
        stream << address.toString() << endl;
    }
}
