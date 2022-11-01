#define BOOST_TEST_MODULE QtGamepad
#include <QtGamepad>
#include <boost/test/unit_test.hpp>
BOOST_AUTO_TEST_CASE( defines)
{
    BOOST_CHECK_EQUAL(BOOST_IS_DEFINED(QT_CORE_LIB), true);
    BOOST_CHECK_EQUAL(BOOST_IS_DEFINED(QT_GAMEPAD_LIB), true);
}
BOOST_AUTO_TEST_CASE( gamepad )
{
    auto gamepads = QGamepadManager::instance()->connectedGamepads();
    if (gamepads.isEmpty()) {
        return;
    }
    QGamepad gamepad(*gamepads.begin());
}
