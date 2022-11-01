#define BOOST_TEST_MODULE QtDeclarative
#include <QCoreApplication>
#include <QDeclarativeView>
#include <boost/test/unit_test.hpp>
BOOST_AUTO_TEST_CASE( defines)
{
    BOOST_CHECK_EQUAL(BOOST_IS_DEFINED(QT_CORE_LIB), true);
    BOOST_CHECK_EQUAL(BOOST_IS_DEFINED(QT_XML_LIB), true);
    BOOST_CHECK_EQUAL(BOOST_IS_DEFINED(QT_DECLARATIVE_LIB), true);
}
BOOST_AUTO_TEST_CASE( declarative )
{
    QCoreApplication app(boost::unit_test::framework::master_test_suite().argc,
                         boost::unit_test::framework::master_test_suite().argv);
    QDeclarativeView view;
}
