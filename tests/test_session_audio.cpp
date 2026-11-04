#include <QtTest>
#include "audio/DeviceSelection.h"
#include "util/Session.h"

using namespace kdict;

class SessionAudioTest : public QObject {
    Q_OBJECT
private slots:
    void sessionClassification()
    {
        QVERIFY(Session::classify("wayland", false, true) == SessionType::Wayland);
        QVERIFY(Session::classify("X11", true, true) == SessionType::X11);
        QVERIFY(Session::classify("tty", true, false) == SessionType::Wayland);
        QVERIFY(Session::classify("", false, true) == SessionType::X11);
        QVERIFY(Session::classify("", false, false) == SessionType::Unknown);
    }
    void deviceSelection()
    {
        const QList<AudioDeviceInfo> devices{
            {"alsa_input.pci", "Built-in Audio Analog Stereo", false},
            {"alsa_input.usb", "USB Audio and HID Mono", true},
        };
        QCOMPARE(selectAudioDevice(devices, "default"), 1);
        QCOMPARE(selectAudioDevice(devices, ""), 1);
        QCOMPARE(selectAudioDevice(devices, "alsa_input.pci"), 0);
        QCOMPARE(selectAudioDevice(devices, "usb audio and hid mono"), 1);
        QCOMPARE(selectAudioDevice(devices, "built-in"), 0);
        QCOMPARE(selectAudioDevice(devices, "nonexistent"), -1);
        QCOMPARE(selectAudioDevice({}, "default"), -1);
    }
};

QTEST_MAIN(SessionAudioTest)
#include "test_session_audio.moc"
