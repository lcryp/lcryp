#define BOOST_TEST_MODULE QtScript
#include <QScriptEngine>
#include <QCoreApplication>
#include <boost/test/unit_test.hpp>
#include <iostream>
std::ostream&
operator << (std::ostream& stream, QString const& string)
{
    stream << qPrintable(string);
    return stream;
}
BOOST_AUTO_TEST_CASE( defines)
{
    BOOST_CHECK_EQUAL(BOOST_IS_DEFINED(QT_SCRIPT_LIB), true);
}
BOOST_AUTO_TEST_CASE( script )
{
    QCoreApplication app(boost::unit_test::framework::master_test_suite().argc,
                         boost::unit_test::framework::master_test_suite().argv);
    QScriptEngine myEngine;
    QScriptValue three = myEngine.evaluate("1 + 2");
    BOOST_CHECK_EQUAL(three.toNumber(), 3);
    BOOST_CHECK_EQUAL(three.toString(), QLatin1String("3"));
}
