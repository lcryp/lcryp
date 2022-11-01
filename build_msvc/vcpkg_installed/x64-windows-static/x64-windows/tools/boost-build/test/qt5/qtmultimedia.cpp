#define BOOST_TEST_MODULE QtMultimedia
#include <QAudioDeviceInfo>
#include <boost/test/unit_test.hpp>
BOOST_AUTO_TEST_CASE( defines)
{
    BOOST_CHECK_EQUAL(BOOST_IS_DEFINED(QT_CORE_LIB), true);
    BOOST_CHECK_EQUAL(BOOST_IS_DEFINED(QT_GUI_LIB), true);
    BOOST_CHECK_EQUAL(BOOST_IS_DEFINED(QT_MULTIMEDIA_LIB), true);
}
BOOST_AUTO_TEST_CASE( audiodevices)
{
    QList<QAudioDeviceInfo> devices = QAudioDeviceInfo::availableDevices(QAudio::AudioOutput);
    for(int i = 0; i < devices.size(); ++i) {
        BOOST_TEST_MESSAGE(QAudioDeviceInfo(devices.at(i)).deviceName().constData());
    }
}
