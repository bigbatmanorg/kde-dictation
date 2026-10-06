#pragma once

#include <QByteArray>

namespace kdict {

class WavEncoder {
public:
    static QByteArray pcm16MonoToWav(const QByteArray &pcm, int sampleRate);
};

} // namespace kdict
