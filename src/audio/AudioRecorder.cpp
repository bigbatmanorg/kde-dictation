#include "audio/AudioRecorder.h"

#include <QAudioDevice>
#include <QAudioSource>
#include <QMediaDevices>
#include <QIODevice>

namespace kdict {

AudioRecorder::AudioRecorder(QObject *parent) : QObject(parent) {}

AudioRecorder::~AudioRecorder() = default;

bool AudioRecorder::start(int sampleRate)
{
    abort();
    pcm_.clear();
    lastError_.clear();

    const QAudioDevice device = QMediaDevices::defaultAudioInput();
    if (device.isNull()) {
        lastError_ = QStringLiteral("No default audio input device available");
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
