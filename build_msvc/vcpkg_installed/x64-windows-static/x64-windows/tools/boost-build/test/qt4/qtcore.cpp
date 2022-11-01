#define BOOST_TEST_MODULE QtCore
#include <QtCore>
#include <boost/test/unit_test.hpp>
BOOST_AUTO_TEST_CASE (defines)
{
    BOOST_CHECK_EQUAL(BOOST_IS_DEFINED(QT_CORE_LIB), true);
}
BOOST_AUTO_TEST_CASE( qstring_test)
{
    QString dummy;
    BOOST_CHECK_EQUAL(dummy.isEmpty(), true);
}
