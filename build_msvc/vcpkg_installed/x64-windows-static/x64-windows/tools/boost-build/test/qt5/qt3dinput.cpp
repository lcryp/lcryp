#define BOOST_TEST_MODULE Qt3DInput
#include <Qt3DInput>
#include <boost/test/unit_test.hpp>
BOOST_AUTO_TEST_CASE (defines)
{
    BOOST_CHECK_EQUAL(BOOST_IS_DEFINED(QT_3DINPUT_LIB), true);
    BOOST_CHECK_EQUAL(BOOST_IS_DEFINED(QT_3DCORE_LIB), true);
    BOOST_CHECK_EQUAL(BOOST_IS_DEFINED(QT_3DRENDER_LIB), true);
}
BOOST_AUTO_TEST_CASE ( sample_code )
{
    Qt3DCore::QEntity rootEntity;
}
