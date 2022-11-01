#define BOOST_TEST_MODULE QtScxml
#include <QtScxml>
#include <boost/test/unit_test.hpp>
BOOST_AUTO_TEST_CASE( defines)
{
    BOOST_CHECK_EQUAL(BOOST_IS_DEFINED(QT_CORE_LIB), true);
    BOOST_CHECK_EQUAL(BOOST_IS_DEFINED(QT_SCXML_LIB), true);
}
std::ostream&
operator << (std::ostream& stream, QString const& string)
{
    stream << qPrintable(string);
    return stream;
}
BOOST_AUTO_TEST_CASE( scxml )
{
    QString sessionId = QScxmlStateMachine::generateSessionId(QStringLiteral("dummy"));
    BOOST_TEST(sessionId.isEmpty() == false);
    BOOST_TEST(sessionId == QString{"dummy1"});
}
