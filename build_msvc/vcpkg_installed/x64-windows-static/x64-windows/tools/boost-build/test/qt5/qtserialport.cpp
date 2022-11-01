#define BOOST_TEST_MODULE QtSerialPort
#include <QtSerialPort>
#include <boost/test/unit_test.hpp>
BOOST_AUTO_TEST_CASE (defines)
{
    BOOST_CHECK_EQUAL(BOOST_IS_DEFINED(QT_CORE_LIB), true);
    BOOST_CHECK_EQUAL(BOOST_IS_DEFINED(QT_SERIALPORT_LIB), true);
}
BOOST_AUTO_TEST_CASE( serialport )
{
    QSerialPort serialPort;
    serialPort.setPortName(QStringLiteral("test serialport"));
}
