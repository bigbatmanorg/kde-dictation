#include <QtTest>
#include "audio/WavEncoder.h"

using namespace kdict;

static quint32 le32(const QByteArray &b, int offset)
{
    return static_cast<quint8>(b[offset])
        | (static_cast<quint32>(static_cast<quint8>(b[offset + 1])) << 8)
        | (static_cast<quint32>(static_cast<quint8>(b[offset + 2])) << 16)
        | (static_cast<quint32>(static_cast<quint8>(b[offset + 3])) << 24);
}

class WavTest : public QObject {
    Q_OBJECT
private slots:
    void createsValidPcmHeader()
    {
        QByteArray pcm(3200, '\0');
        const QByteArray wav = WavEncoder::pcm16MonoToWav(pcm, 16000);
        QCOMPARE(wav.size(), 3244);
        QCOMPARE(wav.mid(0, 4), QByteArray("RIFF"));
        QCOMPARE(wav.mid(8, 4), QByteArray("WAVE"));
        QCOMPARE(wav.mid(36, 4), QByteArray("data"));
        QCOMPARE(le32(wav, 40), quint32(3200));
        QCOMPARE(le32(wav, 24), quint32(16000));
    }
};

QTEST_MAIN(WavTest)
#include "test_wav.moc"
