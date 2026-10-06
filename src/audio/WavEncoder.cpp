#include "audio/WavEncoder.h"

#include <QBuffer>
#include <QDataStream>
#include <stdexcept>

namespace kdict {

QByteArray WavEncoder::pcm16MonoToWav(const QByteArray &pcm, int sampleRate)
{
    if (sampleRate <= 0) {
        throw std::runtime_error("Invalid sample rate");
    }
    if ((pcm.size() % 2) != 0) {
        throw std::runtime_error("PCM16 buffer must contain an even number of bytes");
    }

    QByteArray out;
    out.reserve(44 + pcm.size());
    QBuffer buffer(&out);
    buffer.open(QIODevice::WriteOnly);
    QDataStream s(&buffer);
    s.setByteOrder(QDataStream::LittleEndian);

    const quint16 channels = 1;
    const quint16 bitsPerSample = 16;
    const quint16 blockAlign = channels * bitsPerSample / 8;
    const quint32 byteRate = static_cast<quint32>(sampleRate) * blockAlign;
    const quint32 dataSize = static_cast<quint32>(pcm.size());
    const quint32 riffSize = 36u + dataSize;

    s.writeRawData("RIFF", 4);
    s << riffSize;
    s.writeRawData("WAVE", 4);
    s.writeRawData("fmt ", 4);
    s << quint32(16);       // PCM fmt chunk size
    s << quint16(1);        // PCM format
    s << channels;
    s << quint32(sampleRate);
    s << byteRate;
    s << blockAlign;
    s << bitsPerSample;
    s.writeRawData("data", 4);
    s << dataSize;
    s.writeRawData(pcm.constData(), pcm.size());

    return out;
}

} // namespace kdict
