#define BOOST_TEST_MODULE QtAssistant
#include <QAssistantClient>
#include <boost/test/unit_test.hpp>
BOOST_AUTO_TEST_CASE( defines)
{
    BOOST_CHECK_EQUAL(BOOST_IS_DEFINED(QT_CORE_LIB), true);
    BOOST_CHECK_EQUAL(BOOST_IS_DEFINED(QT_GUI_LIB), true);
}
BOOST_AUTO_TEST_CASE( empty_assistant)
{
    QAssistantClient client(QString());
}
