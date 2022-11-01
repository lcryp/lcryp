#define BOOST_TEST_MODULE QtSerialBus
#include <QtSerialBus>
#include <boost/test/unit_test.hpp>
BOOST_AUTO_TEST_CASE( defines)
{
    BOOST_CHECK_EQUAL(BOOST_IS_DEFINED(QT_CORE_LIB), true);
    BOOST_CHECK_EQUAL(BOOST_IS_DEFINED(QT_SERIALBUS_LIB), true);
}
BOOST_AUTO_TEST_CASE( serialBus )
{
    auto canbus = QCanBus::instance();
    Q_UNUSED(canbus);
}
