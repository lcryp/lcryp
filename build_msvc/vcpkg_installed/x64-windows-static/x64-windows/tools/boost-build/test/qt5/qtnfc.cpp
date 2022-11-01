#define BOOST_TEST_MODULE QtNfc
#include <QtNfc>
#include <boost/test/unit_test.hpp>
BOOST_AUTO_TEST_CASE( defines)
{
    BOOST_CHECK_EQUAL(BOOST_IS_DEFINED(QT_CORE_LIB), true);
    BOOST_CHECK_EQUAL(BOOST_IS_DEFINED(QT_NFC_LIB), true);
}
BOOST_AUTO_TEST_CASE( nfc )
{
    QNearFieldManager manager;
    if (!manager.isAvailable())
    {
        BOOST_TEST_MESSAGE("No Nfc");
    }
}
