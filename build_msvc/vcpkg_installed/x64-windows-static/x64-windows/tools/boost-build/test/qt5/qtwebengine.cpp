#define BOOST_TEST_MODULE QtWebEngine
#include <QtWebEngine>
#include <QGuiApplication>
#include <boost/test/unit_test.hpp>
BOOST_AUTO_TEST_CASE( defines)
{
    BOOST_CHECK_EQUAL(BOOST_IS_DEFINED(QT_CORE_LIB), true);
    BOOST_CHECK_EQUAL(BOOST_IS_DEFINED(QT_GUI_LIB), true);
    BOOST_CHECK_EQUAL(BOOST_IS_DEFINED(QT_WEBENGINE_LIB), true);
}
BOOST_AUTO_TEST_CASE( webengine )
{
    QGuiApplication app(boost::unit_test::framework::master_test_suite().argc,
                        boost::unit_test::framework::master_test_suite().argv);
    QtWebEngine::initialize();
}
