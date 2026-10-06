#pragma once

#include "config/Config.h"
#include <QObject>

class QNetworkAccessManager;
class QNetworkReply;

namespace kdict {

class BatchOpenAIProvider : public QObject {
    Q_OBJECT
public:
    explicit BatchOpenAIProvider(QObject *parent = nullptr);
    void transcribe(const QByteArray &wav, const TranscriptionConfig &config);
    void abort();

signals:
    void finalTranscript(const QString &text);
    void error(const QString &message);

private:
    QNetworkAccessManager *network_;
    QNetworkReply *reply_ = nullptr;
};

} // namespace kdict
