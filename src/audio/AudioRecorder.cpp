#include "audio/AudioRecorder.h"

#include "audio/DeviceSelection.h"

#include <QAudioDevice>
#include <QAudioSource>
#include <QMediaDevices>
#include <QIODevice>

namespace kdict {

AudioRecorder::AudioRecorder(QObject *parent) : QObject(parent) {}

AudioRecorder::~AudioRecorder() = default;

QAudioDevice AudioRecorder::resolveDevice(const QString &wanted)
{
    const QList<QAudioDevice> inputs = QMediaDevices::audioInputs();
    const QByteArray defaultId = QMediaDevices::defaultAudioInput().id();
    QList<AudioDeviceInfo> infos;
    for (const QAudioDevice &d : inputs) {
        infos.append({QString::fromUtf8(d.id()), d.description(), d.id() == defaultId});
    }
    const int index = selectAudioDevice(infos, wanted);
    if (index >= 0) return inputs.at(index);
    // The default may be reported even when audioInputs() omits it.
    const QString w = wanted.trimmed();
    if (w.isEmpty() || w.compare(QStringLiteral("default"), Qt::CaseInsensitive) == 0) {
        return QMediaDevices::defaultAudioInput();
    }
    return {};
}

QStringList AudioRecorder::describeInputs()
{
    QStringList lines;
    const QByteArray defaultId = QMediaDevices::defaultAudioInput().id();
    for (const QAudioDevice &d : QMediaDevices::audioInputs()) {
        lines << QStringLiteral("%1%2  [id: %3]")
                     .arg(d.id() == defaultId ? QStringLiteral("* ") : QStringLiteral("  "),
                          d.description(), QString::fromUtf8(d.id()));
    }
    return lines;
}

bool AudioRecorder::start(int sampleRate)
{
    abort();
    pcm_.clear();
    lastError_.clear();

    const QAudioDevice device = resolveDevice(deviceName_);
    if (device.isNull()) {
        lastError_ = deviceName_.trimmed().isEmpty() || deviceName_.compare(QStringLiteral("default"), Qt::CaseInsensitive) == 0
            ? QStringLiteral("No default audio input device available")
            : QStringLiteral("Audio input '%1' not found (see kde-dictation --list-devices)").arg(deviceName_);
        return false;
    }

    format_.setSampleRate(sampleRate);
    format_.setChannelCount(1);
    format_.setSampleFormat(QAudioFormat::Int16);

    if (!device.isFormatSupported(format_)) {
        lastError_ = QStringLiteral("Default microphone does not accept mono PCM16 at %1 Hz; v1 intentionally has no resampler yet")
                         .arg(sampleRate);
        return false;
    }

    source_ = std::make_unique<QAudioSource>(device, format_);
    device_ = source_->start();
    if (!device_) {
        lastError_ = QStringLiteral("Failed to start audio capture");
        source_.reset();
        return false;
    }

    connect(device_, &QIODevice::readyRead, this, &AudioRecorder::drain);
    connect(source_.get(), &QAudioSource::stateChanged, this, [this](QtAudio::State state) {
        if (state == QtAudio::StoppedState && recording_ && source_ && source_->error() != QtAudio::NoError) {
            emit captureError(QStringLiteral("Audio capture stopped with error code %1").arg(static_cast<int>(source_->error())));
        }
    });

    recording_ = true;
    return true;
}

void AudioRecorder::drain()
{
    if (!device_) return;
    const QByteArray chunk = device_->readAll();
    if (chunk.isEmpty()) return;
    pcm_.append(chunk);
    emit chunkCaptured(chunk);
}

QByteArray AudioRecorder::stop()
{
    if (!recording_) return pcm_;
    drain();
    recording_ = false;
    if (source_) source_->stop();
    drain();
    device_ = nullptr;
    source_.reset();
    return pcm_;
}

void AudioRecorder::abort()
{
    recording_ = false;
    if (source_) source_->stop();
    device_ = nullptr;
    source_.reset();
    pcm_.clear();
}

} // namespace kdict
