#define BOOST_TEST_MODULE QtMoc
#include "mock.h"
#include <boost/test/unit_test.hpp>
Mock::Mock()
{
}
BOOST_AUTO_TEST_CASE(construct_mock)
{
    delete new Mock();
    BOOST_CHECK_EQUAL(BOOST_IS_DEFINED(QT_CORE_LIB), true);
    BOOST_CHECK_EQUAL(BOOST_IS_DEFINED(TEST_MOCK), true);
}
