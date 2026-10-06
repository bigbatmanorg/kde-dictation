#include "stt/BatchOpenAIProvider.h"
#include "util/UrlHelpers.h"

#include <QBuffer>
#include <QHttpMultiPart>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

namespace kdict {
namespace {
QHttpPart textPart(const QByteArray &name, const QByteArray &value)
{
    QHttpPart part;
    part.setHeader(QNetworkRequest::ContentDispositionHeader,
                   QStringLiteral("form-data; name=\"%1\"").arg(QString::fromLatin1(name)));
    part.setBody(value);
    return part;
}
}

BatchOpenAIProvider::BatchOpenAIProvider(QObject *parent)
    : QObject(parent), network_(new QNetworkAccessManager(this)) {}

void BatchOpenAIProvider::abort()
{
    if (reply_) {
        reply_->abort();
        reply_->deleteLater();
        reply_ = nullptr;
    }
}

void BatchOpenAIProvider::transcribe(const QByteArray &wav, const TranscriptionConfig &config)
{
    abort();

    auto *multi = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    multi->append(textPart("model", config.batch.model.toUtf8()));
    multi->append(textPart("response_format", "json"));
    if (!config.batch.language.trimmed().isEmpty() && config.batch.language.toLower() != QStringLiteral("auto")) {
        multi->append(textPart("language", config.batch.language.toUtf8()));
    }

    QHttpPart audioPart;
    audioPart.setHeader(QNetworkRequest::ContentDispositionHeader,
                        QStringLiteral("form-data; name=\"file\"; filename=\"dictation.wav\""));
    audioPart.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("audio/wav"));
    auto *buffer = new QBuffer(multi);
    buffer->setData(wav);
    buffer->open(QIODevice::ReadOnly);
    audioPart.setBodyDevice(buffer);
    multi->append(audioPart);

    QNetworkRequest request(makeHttpEndpoint(config.baseUrl, config.batch.path));
    request.setTransferTimeout(config.batch.timeoutMs);
    const QString key = config.resolvedApiKey();
    if (!key.isEmpty()) request.setRawHeader("Authorization", QByteArray("Bearer ") + key.toUtf8());

    reply_ = network_->post(request, multi);
    multi->setParent(reply_);

    connect(reply_, &QNetworkReply::finished, this, [this]() {
        QNetworkReply *reply = reply_;
        reply_ = nullptr;
        if (!reply) return;

        const QByteArray body = reply->readAll();
        if (reply->error() != QNetworkReply::NoError) {
            const QString msg = QStringLiteral("Batch STT failed: %1; response=%2")
                                    .arg(reply->errorString(), QString::fromUtf8(body.left(1000)));
            reply->deleteLater();
            emit error(msg);
            return;
        }

        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(body, &parseError);
        if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
            reply->deleteLater();
            emit error(QStringLiteral("Batch STT returned invalid JSON: %1").arg(parseError.errorString()));
            return;
        }

        const QString text = doc.object().value(QStringLiteral("text")).toString();
        reply->deleteLater();
        if (text.trimmed().isEmpty()) {
            emit error(QStringLiteral("Batch STT returned an empty transcript"));
            return;
        }
        emit finalTranscript(text);
    });
}

} // namespace kdict
