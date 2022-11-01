#define BOOST_TEST_MODULE QtWebKit
#include <QWebSettings>
#include <boost/test/unit_test.hpp>
BOOST_AUTO_TEST_CASE( defines)
{
    BOOST_CHECK_EQUAL(BOOST_IS_DEFINED(QT_CORE_LIB), true);
    BOOST_CHECK_EQUAL(BOOST_IS_DEFINED(QT_GUI_LIB), true);
    BOOST_CHECK_EQUAL(BOOST_IS_DEFINED(QT_WEBKIT_LIB), true);
}
BOOST_AUTO_TEST_CASE( webkit )
{
    BOOST_CHECK(QWebSettings::globalSettings());
}
