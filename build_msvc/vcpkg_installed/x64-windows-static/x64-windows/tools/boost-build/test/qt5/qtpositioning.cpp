#define BOOST_TEST_MODULE QtPositioning
#include <QGeoCoordinate>
#include <boost/test/unit_test.hpp>
BOOST_AUTO_TEST_CASE (defines)
{
    BOOST_CHECK_EQUAL(BOOST_IS_DEFINED(QT_CORE_LIB), true);
    BOOST_CHECK_EQUAL(BOOST_IS_DEFINED(QT_POSITIONING_LIB), true);
}
BOOST_AUTO_TEST_CASE( geo_coordinate )
{
    QGeoCoordinate geocoordinate;
    BOOST_CHECK_EQUAL(geocoordinate.type(), QGeoCoordinate::InvalidCoordinate);
}
