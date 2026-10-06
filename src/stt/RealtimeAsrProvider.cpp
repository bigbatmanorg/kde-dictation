#include "stt/RealtimeAsrProvider.h"
#include "stt/RealtimeEventParser.h"
#include "util/UrlHelpers.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>
#include <utility>

namespace kdict {

RealtimeAsrProvider::RealtimeAsrProvider(QObject *parent) : QObject(parent)
{
    connectionTimer_.setSingleShot(true);
    finalizationTimer_.setSingleShot(true);

    connect(&socket_, &QWebSocket::textMessageReceived, this, &RealtimeAsrProvider::handleTextMessage);
    connect(&socket_, &QWebSocket::errorOccurred, this, [this](QAbstractSocket::SocketError) {
        if (active_) fail(QStringLiteral("Realtime WebSocket error: %1").arg(socket_.errorString()));
    });
    connect(&socket_, &QWebSocket::disconnected, this, [this]() {
        if (active_ && !commitSent_) fail(QStringLiteral("Realtime WebSocket disconnected unexpectedly"));
    });
    connect(&connectionTimer_, &QTimer::timeout, this, [this]() {
        fail(QStringLiteral("Realtime session setup timed out"));
    });
    connect(&finalizationTimer_, &QTimer::timeout, this, [this]() {
        fail(QStringLiteral("Realtime finalization timed out after %1 ms").arg(config_.realtime.finalizationTimeoutMs));
    });
}

void RealtimeAsrProvider::start(const TranscriptionConfig &config, int sampleRate)
{
    abort();
    config_ = config;
    sampleRate_ = sampleRate;
    active_ = true;

    QNetworkRequest request(makeWebSocketEndpoint(config_.baseUrl, config_.realtime.path));
    const QString key = config_.resolvedApiKey();
    if (!key.isEmpty()) request.setRawHeader("Authorization", QByteArray("Bearer ") + key.toUtf8());
    socket_.open(request);
    connectionTimer_.start(qMax(3000, config_.realtime.finalizationTimeoutMs));
}

void RealtimeAsrProvider::pushAudio(const QByteArray &pcm16)
{
    if (!active_ || pcm16.isEmpty()) return;
    if (sessionReady_) {
        socket_.sendBinaryMessage(pcm16);
    } else {
        pendingAudio_.append(pcm16);
    }
}

void RealtimeAsrProvider::finish()
{
    if (!active_) {
        emit error(QStringLiteral("Realtime provider is not active"));
        return;
    }
    finishRequested_ = true;
    if (sessionReady_) sendCommit();
}

void RealtimeAsrProvider::abort()
{
    connectionTimer_.stop();
    finalizationTimer_.stop();
    active_ = false;
    sessionReady_ = false;
    finishRequested_ = false;
    commitSent_ = false;
    pendingAudio_.clear();
    lastCompletedText_.clear();
    if (socket_.state() != QAbstractSocket::UnconnectedState) socket_.abort();
}

void RealtimeAsrProvider::sendSessionUpdate()
{
    QJsonObject session;
    session.insert(QStringLiteral("sample_rate"), sampleRate_);
    session.insert(QStringLiteral("automatic_punctuation"), config_.realtime.automaticPunctuation);
    if (!config_.realtime.language.trimmed().isEmpty() && config_.realtime.language.toLower() != QStringLiteral("auto")) {
        session.insert(QStringLiteral("language"), config_.realtime.language);
    }

    QJsonObject root;
    root.insert(QStringLiteral("type"), QStringLiteral("session.update"));
    root.insert(QStringLiteral("session"), session);
    socket_.sendTextMessage(QString::fromUtf8(QJsonDocument(root).toJson(QJsonDocument::Compact)));
}

void RealtimeAsrProvider::flushPendingAudio()
{
    for (const auto &chunk : std::as_const(pendingAudio_)) socket_.sendBinaryMessage(chunk);
    pendingAudio_.clear();
}

void RealtimeAsrProvider::sendCommit()
{
    if (commitSent_) return;
    commitSent_ = true;
    const QByteArray json = R"({"type":"input_audio_buffer.commit"})";
    socket_.sendTextMessage(QString::fromUtf8(json));
    finalizationTimer_.start(config_.realtime.finalizationTimeoutMs);
}

void RealtimeAsrProvider::fail(const QString &message)
{
    if (!active_) return;
    connectionTimer_.stop();
    finalizationTimer_.stop();
    active_ = false;
    if (socket_.state() != QAbstractSocket::UnconnectedState) socket_.abort();
    emit error(message);
}

void RealtimeAsrProvider::handleTextMessage(const QString &message)
{
    if (!active_) return;
    const auto event = RealtimeEventParser::parse(message);

    switch (event.kind) {
    case RealtimeEventKind::SessionCreated:
        sendSessionUpdate();
        break;
    case RealtimeEventKind::SessionUpdated:
        connectionTimer_.stop();
        sessionReady_ = true;
        flushPendingAudio();
        if (finishRequested_) sendCommit();
        break;
    case RealtimeEventKind::Delta:
        if (!event.text.isEmpty()) emit partialTranscript(event.text);
        break;
    case RealtimeEventKind::Completed:
        if (!event.text.trimmed().isEmpty()) lastCompletedText_ = event.text;
        if (finishRequested_ && !lastCompletedText_.trimmed().isEmpty()) {
            finalizationTimer_.stop();
            const QString finalText = lastCompletedText_;
            active_ = false;
            socket_.close();
            emit finalTranscript(finalText);
        }
        break;
    case RealtimeEventKind::Committed:
        // Completion normally follows. Keep waiting until the configured timeout.
        break;
    case RealtimeEventKind::Error:
        fail(event.errorMessage.isEmpty() ? QStringLiteral("Realtime server returned an error") : event.errorMessage);
        break;
    case RealtimeEventKind::Unknown:
        break;
    }
}

} // namespace kdict
