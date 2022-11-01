#define BOOST_TEST_MODULE QtSvg
#include <QtSvg>
#include <boost/test/unit_test.hpp>
BOOST_AUTO_TEST_CASE( defines)
{
    BOOST_CHECK_EQUAL(BOOST_IS_DEFINED(QT_CORE_LIB), true);
    BOOST_CHECK_EQUAL(BOOST_IS_DEFINED(QT_SVG_LIB), true);
}
BOOST_AUTO_TEST_CASE( generator_construct)
{
    QSvgGenerator generator;
}
