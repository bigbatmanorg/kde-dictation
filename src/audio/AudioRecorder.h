#pragma once

#include <QObject>
#include <QByteArray>
#include <QAudioDevice>
#include <QAudioFormat>
#include <memory>

class QAudioSource;
class QIODevice;

namespace kdict {

class AudioRecorder : public QObject {
    Q_OBJECT
public:
    explicit AudioRecorder(QObject *parent = nullptr);
    ~AudioRecorder() override;

    // Resolves audio.device ("default", id or description fragment) to a device.
    // A null device is returned when the name does not match any input.
    static QAudioDevice resolveDevice(const QString &wanted);
    static QStringList describeInputs();

    void setDeviceName(const QString &name) { deviceName_ = name; }
    bool start(int sampleRate);
    QByteArray stop();
    void abort();
    bool isRecording() const { return recording_; }
    QString lastError() const { return lastError_; }
    int sampleRate() const { return format_.sampleRate(); }

signals:
    void chunkCaptured(const QByteArray &pcm16);
    void captureError(const QString &message);

private slots:
    void drain();

private:
    std::unique_ptr<QAudioSource> source_;
    QIODevice *device_ = nullptr;
    QByteArray pcm_;
    QAudioFormat format_;
    bool recording_ = false;
    QString lastError_;
    QString deviceName_ = QStringLiteral("default");
};

} // namespace kdict
