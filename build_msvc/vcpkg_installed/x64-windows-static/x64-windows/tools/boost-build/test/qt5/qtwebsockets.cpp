#define BOOST_TEST_MODULE QtWebSockets
#include <QtWebSockets>
#include <boost/test/unit_test.hpp>
BOOST_AUTO_TEST_CASE (defines)
{
    BOOST_CHECK_EQUAL(BOOST_IS_DEFINED(QT_CORE_LIB), true);
    BOOST_CHECK_EQUAL(BOOST_IS_DEFINED(QT_WEBSOCKETS_LIB), true);
}
BOOST_AUTO_TEST_CASE( websocket )
{
    QWebSocket socket;
    socket.setPauseMode(QAbstractSocket::PauseNever);
    BOOST_TEST(socket.isValid() == false);
}
