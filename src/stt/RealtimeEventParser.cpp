#include "stt/RealtimeEventParser.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

namespace kdict {
namespace {

QString firstText(const QJsonObject &obj)
{
    static const char *keys[] = {"transcript", "text", "delta"};
    for (const char *key : keys) {
        const auto value = obj.value(QLatin1String(key));
        if (value.isString() && !value.toString().isEmpty()) return value.toString();
    }
    const auto item = obj.value(QStringLiteral("item"));
    if (item.isObject()) {
        const QString nested = firstText(item.toObject());
        if (!nested.isEmpty()) return nested;
    }
    return {};
}

QString errorText(const QJsonObject &obj)
{
    const auto error = obj.value(QStringLiteral("error"));
    if (error.isObject()) {
        const auto e = error.toObject();
        const auto message = e.value(QStringLiteral("message"));
        if (message.isString()) return message.toString();
    }
    return firstText(obj);
}

} // namespace

RealtimeEvent RealtimeEventParser::parse(const QString &message)
{
    RealtimeEvent event;
    QJsonParseError parseError;
    const auto doc = QJsonDocument::fromJson(message.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        event.kind = RealtimeEventKind::Error;
        event.errorMessage = QStringLiteral("Invalid realtime JSON: %1").arg(parseError.errorString());
        return event;
    }

    const auto obj = doc.object();
    event.type = obj.value(QStringLiteral("type")).toString();

    if (event.type == QStringLiteral("session.created")) {
        event.kind = RealtimeEventKind::SessionCreated;
    } else if (event.type == QStringLiteral("session.updated")) {
        event.kind = RealtimeEventKind::SessionUpdated;
    } else if (event.type == QStringLiteral("transcription.delta")
               || event.type.endsWith(QStringLiteral("input_audio_transcription.delta"))) {
        event.kind = RealtimeEventKind::Delta;
        event.text = firstText(obj);
    } else if (event.type == QStringLiteral("transcription.done")
               || event.type.endsWith(QStringLiteral("input_audio_transcription.completed"))) {
        event.kind = RealtimeEventKind::Completed;
        event.text = firstText(obj);
    } else if (event.type == QStringLiteral("input_audio_buffer.committed")) {
        event.kind = RealtimeEventKind::Committed;
    } else if (event.type == QStringLiteral("error")) {
        event.kind = RealtimeEventKind::Error;
        event.errorMessage = errorText(obj);
    }

    return event;
}

} // namespace kdict
