#include <QtTest>
class QtTest: public QObject
{
#if defined(TEST_MOCK)
    Q_OBJECT
#endif
private Q_SLOTS:
    void toUpper();
};
void
QtTest::toUpper()
{
    QString str = "Hello";
    QCOMPARE(str.toUpper(), QString("HELLO"));
}
QTEST_MAIN(QtTest)
#include "qttest.moc"
