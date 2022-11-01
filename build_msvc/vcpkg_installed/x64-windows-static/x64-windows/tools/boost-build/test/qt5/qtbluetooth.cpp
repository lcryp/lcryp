#define BOOST_TEST_MODULE QtBluetooth
#include <QtBluetooth>
#include <boost/test/unit_test.hpp>
BOOST_AUTO_TEST_CASE( defines)
{
    BOOST_CHECK_EQUAL(BOOST_IS_DEFINED(QT_CORE_LIB), true);
    BOOST_CHECK_EQUAL(BOOST_IS_DEFINED(QT_BLUETOOTH_LIB), true);
}
BOOST_AUTO_TEST_CASE( bluetooth )
{
    QList<QBluetoothHostInfo> localAdapters = QBluetoothLocalDevice::allDevices();
    if (!localAdapters.empty())
    {
        QBluetoothLocalDevice adapter(localAdapters.at(0).address());
        adapter.setHostMode(QBluetoothLocalDevice::HostDiscoverable);
    }
    else
    {
        BOOST_TEST(localAdapters.size() == 0);
    }
}
