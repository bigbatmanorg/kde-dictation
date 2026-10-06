#include "stt/RealtimeAsrProvider.h"
#include "stt/RealtimeEventParser.h"
#include "util/UrlHelpers.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>
#include <QNetworkProxy>
#include <QUrlQuery>
#include <utility>

namespace kdict {

RealtimeAsrProvider::RealtimeAsrProvider(QObject *parent) : QObject(parent)
{
    // Realtime ASR endpoints are often private LAN services.  Do not let a
    // desktop/system proxy turn a direct WebSocket connection into a 403.
    socket_.setProxy(QNetworkProxy::NoProxy);
    connectionTimer_.setSingleShot(true);
    finalizationTimer_.setSingleShot(true);

    connect(&socket_, &QWebSocket::textMessageReceived, this, &RealtimeAsrProvider::handleTextMessage);
    connect(&socket_, &QWebSocket::errorOccurred, this, [this](QAbstractSocket::SocketError) {
        if (active_) fail(QStringLiteral("Realtime WebSocket error: %1").arg(socket_.errorString()));
    });
    connect(&socket_, &QWebSocket::disconnected, this, [this]() {
        // If the socket disappears before we have emitted finalTranscript, fail
        // immediately so the controller can use its retained-audio batch fallback.
        if (active_) fail(QStringLiteral("Realtime WebSocket disconnected unexpectedly"));
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

    QUrl endpoint = makeWebSocketEndpoint(config_.baseUrl, config_.realtime.path);
    QUrlQuery query(endpoint);
    query.addQueryItem(QStringLiteral("model"), config_.realtime.model);
    endpoint.setQuery(query);
    QNetworkRequest request(endpoint);
    const QString key = config_.resolvedApiKey();
    if (!key.isEmpty()) request.setRawHeader("Authorization", QByteArray("Bearer ") + key.toUtf8());
    socket_.open(request);
    connectionTimer_.start(qMax(3000, config_.realtime.finalizationTimeoutMs));
}

void RealtimeAsrProvider::pushAudio(const QByteArray &pcm16)
{
    if (!active_ || pcm16.isEmpty()) return;
    if (sessionReady_) {
        QJsonObject event;
        event.insert(QStringLiteral("type"), QStringLiteral("input_audio_buffer.append"));
        event.insert(QStringLiteral("audio"), QString::fromLatin1(pcm16.toBase64()));
        socket_.sendTextMessage(QString::fromUtf8(QJsonDocument(event).toJson(QJsonDocument::Compact)));
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
    QJsonObject root;
    root.insert(QStringLiteral("type"), QStringLiteral("session.update"));
    root.insert(QStringLiteral("model"), config_.realtime.model);
    socket_.sendTextMessage(QString::fromUtf8(QJsonDocument(root).toJson(QJsonDocument::Compact)));
}

void RealtimeAsrProvider::flushPendingAudio()
{
    for (const auto &chunk : std::as_const(pendingAudio_)) {
        QJsonObject event;
        event.insert(QStringLiteral("type"), QStringLiteral("input_audio_buffer.append"));
        event.insert(QStringLiteral("audio"), QString::fromLatin1(chunk.toBase64()));
        socket_.sendTextMessage(QString::fromUtf8(QJsonDocument(event).toJson(QJsonDocument::Compact)));
    }
    pendingAudio_.clear();
}

void RealtimeAsrProvider::sendCommit()
{
    if (commitSent_) return;
    commitSent_ = true;
    QJsonObject event;
    event.insert(QStringLiteral("type"), QStringLiteral("input_audio_buffer.commit"));
    event.insert(QStringLiteral("final"), true);
    socket_.sendTextMessage(QString::fromUtf8(QJsonDocument(event).toJson(QJsonDocument::Compact)));
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
        // vLLM accepts audio immediately after session.update and does not
        // guarantee a session.updated acknowledgement before deltas arrive.
        connectionTimer_.stop();
        sessionReady_ = true;
        socket_.sendTextMessage(QStringLiteral(R"({"type":"input_audio_buffer.commit"})"));
        flushPendingAudio();
        if (finishRequested_) sendCommit();
        break;
    case RealtimeEventKind::SessionUpdated:
        connectionTimer_.stop();
        // Some servers send this acknowledgement; vLLM may not.
        if (!sessionReady_) {
            sessionReady_ = true;
            socket_.sendTextMessage(QStringLiteral(R"({"type":"input_audio_buffer.commit"})"));
            flushPendingAudio();
        }
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
