#pragma once

#include "config/Config.h"
#include <QObject>
#include <QList>
#include <QByteArray>
#include <QWebSocket>
#include <QTimer>

namespace kdict {

class RealtimeAsrProvider : public QObject {
    Q_OBJECT
public:
    explicit RealtimeAsrProvider(QObject *parent = nullptr);

    void start(const TranscriptionConfig &config, int sampleRate);
    void pushAudio(const QByteArray &pcm16);
    void finish();
    void abort();
    bool isActive() const { return active_; }

signals:
    void partialTranscript(const QString &text);
    void finalTranscript(const QString &text);
    void error(const QString &message);

private:
    void sendSessionUpdate();
    void flushPendingAudio();
    void sendCommit();
    void fail(const QString &message);
    void handleTextMessage(const QString &message);

    QWebSocket socket_;
    QTimer connectionTimer_;
    QTimer finalizationTimer_;
    TranscriptionConfig config_;
    int sampleRate_ = 16000;
    QList<QByteArray> pendingAudio_;
    QString lastCompletedText_;
    bool active_ = false;
    bool sessionReady_ = false;
    bool finishRequested_ = false;
    bool commitSent_ = false;
};

} // namespace kdict
