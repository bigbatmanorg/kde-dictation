#include "config/Config.h"

#include <QFileInfo>
#include <QProcessEnvironment>
#include <QStandardPaths>
#include <stdexcept>
#include <yaml-cpp/yaml.h>

namespace kdict {
namespace {

QString qstr(const YAML::Node &node, const char *key, const QString &fallback)
{
    if (!node || !node[key]) {
        return fallback;
    }
    return QString::fromStdString(node[key].as<std::string>());
}

int qint(const YAML::Node &node, const char *key, int fallback)
{
    return node && node[key] ? node[key].as<int>() : fallback;
}

bool qbool(const YAML::Node &node, const char *key, bool fallback)
{
    return node && node[key] ? node[key].as<bool>() : fallback;
}

ActivationMode activationModeFromString(const QString &value)
{
    const auto normalized = value.trimmed().toLower();
    if (normalized == QStringLiteral("toggle")) {
        return ActivationMode::Toggle;
    }
    if (normalized == QStringLiteral("push_to_talk") || normalized == QStringLiteral("push-to-talk")) {
        return ActivationMode::PushToTalk;
    }
    throw std::runtime_error(QStringLiteral("Unsupported activation.mode: %1").arg(value).toStdString());
}

TranscriptionMode transcriptionModeFromString(const QString &value)
{
    const auto normalized = value.trimmed().toLower();
    if (normalized == QStringLiteral("batch")) {
        return TranscriptionMode::Batch;
    }
    if (normalized == QStringLiteral("realtime")) {
        return TranscriptionMode::Realtime;
    }
    throw std::runtime_error(QStringLiteral("Unsupported transcription.mode: %1").arg(value).toStdString());
}

} // namespace

QString TranscriptionConfig::resolvedApiKey() const
{
    if (!apiKeyEnv.trimmed().isEmpty()) {
        const QByteArray value = qgetenv(apiKeyEnv.toUtf8().constData());
        if (!value.isEmpty()) {
            return QString::fromUtf8(value);
        }
    }
    return apiKey;
}

QString ConfigLoader::defaultPath()
{
    return QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
        + QStringLiteral("/kde-dictation/config.yaml");
}

AppConfig ConfigLoader::load(const QString &path)
{
    if (!QFileInfo::exists(path)) {
        throw std::runtime_error(QStringLiteral("Config file not found: %1").arg(path).toStdString());
    }

    const YAML::Node root = YAML::LoadFile(path.toStdString());
    AppConfig cfg;

    const auto activation = root["activation"];
    cfg.activationMode = activationModeFromString(qstr(activation, "mode", QStringLiteral("toggle")));

    const auto audio = root["audio"];
    cfg.audio.device = qstr(audio, "device", cfg.audio.device);
    cfg.audio.sampleRate = qint(audio, "sample_rate", cfg.audio.sampleRate);
    cfg.audio.maxRecordingSeconds = qint(audio, "max_recording_seconds", cfg.audio.maxRecordingSeconds);

    const auto tx = root["transcription"];
    cfg.transcription.mode = transcriptionModeFromString(qstr(tx, "mode", QStringLiteral("batch")));
    cfg.transcription.baseUrl = QUrl(qstr(tx, "base_url", QString()));
    cfg.transcription.apiKeyEnv = qstr(tx, "api_key_env", cfg.transcription.apiKeyEnv);
    cfg.transcription.apiKey = qstr(tx, "api_key", QString());

    const auto batch = tx["batch"];
    cfg.transcription.batch.model = qstr(batch, "model", cfg.transcription.batch.model);
    cfg.transcription.batch.path = qstr(batch, "path", cfg.transcription.batch.path);
    cfg.transcription.batch.language = qstr(batch, "language", cfg.transcription.batch.language);
    cfg.transcription.batch.timeoutMs = qint(batch, "timeout_ms", cfg.transcription.batch.timeoutMs);

    const auto realtime = tx["realtime"];
    cfg.transcription.realtime.path = qstr(realtime, "path", cfg.transcription.realtime.path);
    cfg.transcription.realtime.model = qstr(realtime, "model", cfg.transcription.realtime.model);
    cfg.transcription.realtime.language = qstr(realtime, "language", cfg.transcription.realtime.language);
    cfg.transcription.realtime.automaticPunctuation = qbool(realtime, "automatic_punctuation", cfg.transcription.realtime.automaticPunctuation);
    cfg.transcription.realtime.fallbackToBatch = qbool(realtime, "fallback_to_batch", cfg.transcription.realtime.fallbackToBatch);
    cfg.transcription.realtime.finalizationTimeoutMs = qint(realtime, "finalization_timeout_ms", cfg.transcription.realtime.finalizationTimeoutMs);
    cfg.transcription.realtime.liveInsert = qbool(realtime, "live_insert", cfg.transcription.realtime.liveInsert);
    cfg.transcription.realtime.liveFlushMs = qint(realtime, "live_flush_ms", cfg.transcription.realtime.liveFlushMs);

    const auto output = root["output"];
    cfg.output.method = qstr(output, "method", cfg.output.method);
    cfg.output.pasteShortcut = qstr(output, "paste_shortcut", cfg.output.pasteShortcut);
    cfg.output.pasteDelayMs = qint(output, "paste_delay_ms", cfg.output.pasteDelayMs);
    cfg.output.livePasteDelayMs = qint(output, "live_paste_delay_ms", cfg.output.livePasteDelayMs);

    const auto logging = root["logging"];
    cfg.logging.logTranscripts = qbool(logging, "log_transcripts", cfg.logging.logTranscripts);

    validate(cfg);
    return cfg;
}

void ConfigLoader::validate(const AppConfig &cfg)
{
    if (!cfg.transcription.baseUrl.isValid() || cfg.transcription.baseUrl.scheme().isEmpty() || cfg.transcription.baseUrl.host().isEmpty()) {
        throw std::runtime_error("transcription.base_url must be a valid absolute URL");
    }
    if (cfg.audio.sampleRate < 8000 || cfg.audio.sampleRate > 96000) {
        throw std::runtime_error("audio.sample_rate must be between 8000 and 96000");
    }
    if (cfg.audio.maxRecordingSeconds <= 0) {
        throw std::runtime_error("audio.max_recording_seconds must be greater than zero");
    }
    if (cfg.transcription.batch.model.trimmed().isEmpty()) {
        throw std::runtime_error("transcription.batch.model cannot be empty");
    }
    if (cfg.output.method != QStringLiteral("clipboard_paste")) {
        throw std::runtime_error("Only output.method=clipboard_paste is supported in v1");
    }
    const auto paste = cfg.output.pasteShortcut.trimmed().toLower();
    if (paste != QStringLiteral("ctrl+v") && paste != QStringLiteral("shift+insert")) {
        throw std::runtime_error("output.paste_shortcut must be ctrl+v or shift+insert");
    }
    if (cfg.output.pasteDelayMs < 0 || cfg.output.pasteDelayMs > 5000) {
        throw std::runtime_error("output.paste_delay_ms must be between 0 and 5000");
    }
    if (cfg.transcription.realtime.finalizationTimeoutMs < 250) {
        throw std::runtime_error("realtime.finalization_timeout_ms must be at least 250");
    }
    if (cfg.transcription.realtime.model.trimmed().isEmpty()) {
        throw std::runtime_error("realtime.model cannot be empty");
    }
    if (cfg.transcription.realtime.liveFlushMs < 0 || cfg.transcription.realtime.liveFlushMs > 2000) {
        throw std::runtime_error("realtime.live_flush_ms must be between 0 and 2000");
    }
    if (cfg.output.livePasteDelayMs < 0 || cfg.output.livePasteDelayMs > 1000) {
        throw std::runtime_error("output.live_paste_delay_ms must be between 0 and 1000");
    }
}

QString toString(ActivationMode mode)
{
    return mode == ActivationMode::Toggle ? QStringLiteral("toggle") : QStringLiteral("push_to_talk");
}

QString toString(TranscriptionMode mode)
{
    return mode == TranscriptionMode::Batch ? QStringLiteral("batch") : QStringLiteral("realtime");
}

} // namespace kdict
