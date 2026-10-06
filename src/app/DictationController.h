#pragma once

#include "app/DictationState.h"
#include "config/Config.h"
#include <QObject>
#include <QByteArray>
#include <QTimer>

namespace kdict {

class AudioRecorder;
class BatchOpenAIProvider;
class RealtimeAsrProvider;
class TextOutput;
class ShortcutController;
class TrayController;

class DictationController : public QObject {
    Q_OBJECT
public:
    DictationController(const QString &configPath,
                        AudioRecorder *audio,
                        BatchOpenAIProvider *batch,
                        RealtimeAsrProvider *realtime,
                        TextOutput *output,
                        ShortcutController *shortcut,
                        TrayController *tray,
                        QObject *parent = nullptr);

    bool initialize(QString *errorMessage = nullptr);

public slots:
    void startRecording();
    void stopRecording();
    void toggleRecording();
    void reloadConfig();

private:
    void startBatchFallback(const QString &reason);
    void startBatch(const QByteArray &pcm);
    void handlePartialTranscript(const QString &text);
    void handleFinalTranscript(const QString &text);
    void handleRealtimeError(const QString &message);
    void fail(const QString &message);
    bool loadConfig(QString *errorMessage = nullptr);

    QString configPath_;
    AppConfig config_;
    DictationStateMachine state_;
    AudioRecorder *audio_;
    BatchOpenAIProvider *batch_;
    RealtimeAsrProvider *realtime_;
    TextOutput *output_;
    ShortcutController *shortcut_;
    TrayController *tray_;
    QByteArray capturedPcm_;
    bool realtimeFailed_ = false;
    QString streamedText_;
    QTimer maxRecordingTimer_;
};

} // namespace kdict
