#define BOOST_TEST_MODULE QtPhonon
#include <phonon/MediaObject>
#include <boost/test/unit_test.hpp>
BOOST_AUTO_TEST_CASE( defines)
{
    BOOST_CHECK_EQUAL(BOOST_IS_DEFINED(QT_CORE_LIB), true);
    BOOST_CHECK_EQUAL(BOOST_IS_DEFINED(QT_GUI_LIB), true);
    BOOST_CHECK_EQUAL(BOOST_IS_DEFINED(QT_XML_LIB), true);
    BOOST_CHECK_EQUAL(BOOST_IS_DEFINED(QT_PHONON_LIB), true);
}
BOOST_AUTO_TEST_CASE( phonon_object)
{
    Phonon::MediaObject player;
}
