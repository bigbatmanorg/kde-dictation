#pragma once

#include <QUrl>
#include <QString>

namespace kdict {

enum class ActivationMode { Toggle, PushToTalk };
enum class TranscriptionMode { Batch, Realtime };

struct AudioConfig {
    QString device = QStringLiteral("default");
    int sampleRate = 16000;
    int maxRecordingSeconds = 900;
};

struct BatchConfig {
    QString model = QStringLiteral("whisper-1");
    QString path = QStringLiteral("/v1/audio/transcriptions");
    QString language = QStringLiteral("auto");
    int timeoutMs = 1200000;
};

struct RealtimeConfig {
    QString path = QStringLiteral("/v1/realtime");
    QString model = QStringLiteral("asr-realtime");
    QString language = QStringLiteral("auto");
    bool automaticPunctuation = true;
    bool fallbackToBatch = true;
    int finalizationTimeoutMs = 5000;
};

struct TranscriptionConfig {
    TranscriptionMode mode = TranscriptionMode::Batch;
    QUrl baseUrl;
    QString apiKeyEnv = QStringLiteral("LITELLM_API_KEY");
    QString apiKey;
    BatchConfig batch;
    RealtimeConfig realtime;

    QString resolvedApiKey() const;
};

struct OutputConfig {
    QString method = QStringLiteral("clipboard_paste");
    QString pasteShortcut = QStringLiteral("ctrl+v");
    int pasteDelayMs = 80;
};

struct LoggingConfig {
    bool logTranscripts = false;
};

struct AppConfig {
    ActivationMode activationMode = ActivationMode::Toggle;
    AudioConfig audio;
    TranscriptionConfig transcription;
    OutputConfig output;
    LoggingConfig logging;
};

class ConfigLoader {
public:
    static QString defaultPath();
    static AppConfig load(const QString &path);
    static void validate(const AppConfig &config);
};

QString toString(ActivationMode mode);
QString toString(TranscriptionMode mode);

} // namespace kdict
