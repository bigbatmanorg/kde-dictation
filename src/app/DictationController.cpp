#include "app/DictationController.h"

#include "app/FallbackPolicy.h"
#include "audio/AudioRecorder.h"
#include "audio/WavEncoder.h"
#include "output/TextOutput.h"
#include "shortcut/ShortcutController.h"
#include "stt/BatchOpenAIProvider.h"
#include "stt/RealtimeAsrProvider.h"
#include "ui/TrayController.h"

#include <QDebug>
#include <exception>

namespace kdict {

DictationController::DictationController(const QString &configPath,
                                         AudioRecorder *audio,
                                         BatchOpenAIProvider *batch,
                                         RealtimeAsrProvider *realtime,
                                         TextOutput *output,
                                         ShortcutController *shortcut,
                                         TrayController *tray,
                                         QObject *parent)
    : QObject(parent), configPath_(configPath), audio_(audio), batch_(batch), realtime_(realtime),
      output_(output), shortcut_(shortcut), tray_(tray)
{
    maxRecordingTimer_.setSingleShot(true);
    connect(&maxRecordingTimer_, &QTimer::timeout, this, &DictationController::stopRecording);

    connect(shortcut_, &ShortcutController::startRequested, this, &DictationController::startRecording);
    connect(shortcut_, &ShortcutController::stopRequested, this, &DictationController::stopRecording);
    connect(shortcut_, &ShortcutController::toggleRequested, this, &DictationController::toggleRecording);
    connect(tray_, &TrayController::toggleRequested, this, &DictationController::toggleRecording);
    connect(tray_, &TrayController::reloadRequested, this, &DictationController::reloadConfig);

    connect(audio_, &AudioRecorder::chunkCaptured, this, [this](const QByteArray &chunk) {
        if (config_.transcription.mode == TranscriptionMode::Realtime && !realtimeFailed_) {
            realtime_->pushAudio(chunk);
        }
    });
    connect(audio_, &AudioRecorder::captureError, this, [this](const QString &message) {
        if (state_.state() == DictationState::Recording) fail(message);
    });

    connect(batch_, &BatchOpenAIProvider::finalTranscript, this, &DictationController::handleFinalTranscript);
    connect(batch_, &BatchOpenAIProvider::error, this, &DictationController::fail);
    connect(realtime_, &RealtimeAsrProvider::finalTranscript, this, &DictationController::handleFinalTranscript);
    connect(realtime_, &RealtimeAsrProvider::error, this, &DictationController::handleRealtimeError);

    connect(output_, &TextOutput::warning, this, [this](const QString &message) {
        tray_->showInfo(message);
    });
    connect(output_, &TextOutput::finished, this, [this]() {
        state_.complete();
        tray_->showIdle();
    });
}

bool DictationController::initialize(QString *errorMessage)
{
    return loadConfig(errorMessage);
}

bool DictationController::loadConfig(QString *errorMessage)
{
    try {
        const AppConfig loaded = ConfigLoader::load(configPath_);
        if (state_.state() != DictationState::Idle) {
            if (errorMessage) *errorMessage = QStringLiteral("Configuration can only be reloaded while idle");
            return false;
        }
        config_ = loaded;
        shortcut_->setMode(config_.activationMode);
        tray_->showIdle();
        qInfo().noquote() << "Loaded config" << configPath_
                          << "activation=" << toString(config_.activationMode)
                          << "transcription=" << toString(config_.transcription.mode);
        return true;
    } catch (const std::exception &e) {
        if (errorMessage) *errorMessage = QString::fromUtf8(e.what());
        return false;
    }
}

void DictationController::reloadConfig()
{
    QString error;
    if (!loadConfig(&error)) tray_->showError(error);
    else tray_->showInfo(QStringLiteral("Configuration reloaded"));
}

void DictationController::toggleRecording()
{
    if (state_.state() == DictationState::Idle) startRecording();
    else if (state_.state() == DictationState::Recording) stopRecording();
}

void DictationController::startRecording()
{
    if (!state_.beginRecording()) return;

    capturedPcm_.clear();
    realtimeFailed_ = false;

    if (config_.transcription.mode == TranscriptionMode::Realtime) {
        realtime_->start(config_.transcription, config_.audio.sampleRate);
    }

    if (!audio_->start(config_.audio.sampleRate)) {
        realtime_->abort();
        fail(audio_->lastError());
        return;
    }

    maxRecordingTimer_.start(config_.audio.maxRecordingSeconds * 1000);
    tray_->showRecording();
    qInfo() << "Recording started";
}

void DictationController::stopRecording()
{
    if (!state_.beginFinalizing()) return;

    maxRecordingTimer_.stop();
    capturedPcm_ = audio_->stop();
    tray_->showFinalizing();
    qInfo() << "Recording stopped, PCM bytes=" << capturedPcm_.size();

    if (capturedPcm_.isEmpty()) {
        realtime_->abort();
        fail(QStringLiteral("No audio was captured"));
        return;
    }

    if (config_.transcription.mode == TranscriptionMode::Batch || realtimeFailed_) {
        startBatch(capturedPcm_);
        return;
    }

    realtime_->finish();
}

void DictationController::handleRealtimeError(const QString &message)
{
    const auto action = FallbackPolicy::onRealtimeFailure(state_.state(), config_.transcription.realtime.fallbackToBatch);
    if (action == FallbackAction::DeferUntilStop) {
        realtimeFailed_ = true;
        realtime_->abort();
        tray_->showInfo(QStringLiteral("Realtime unavailable; this recording will use batch transcription on stop"));
        qWarning().noquote() << message;
        return;
    }
    if (action == FallbackAction::StartBatchNow) {
        startBatchFallback(message);
        return;
    }
    fail(message);
}

void DictationController::startBatchFallback(const QString &reason)
{
    realtimeFailed_ = true;
    realtime_->abort();
    tray_->showFinalizing(QStringLiteral("Realtime failed; falling back to batch…"));
    qWarning().noquote() << "Realtime fallback:" << reason;
    startBatch(capturedPcm_);
}

void DictationController::startBatch(const QByteArray &pcm)
{
    try {
        const QByteArray wav = WavEncoder::pcm16MonoToWav(pcm, config_.audio.sampleRate);
        batch_->transcribe(wav, config_.transcription);
    } catch (const std::exception &e) {
        fail(QString::fromUtf8(e.what()));
    }
}

void DictationController::handleFinalTranscript(const QString &text)
{
    if (state_.state() != DictationState::Finalizing) return;
    const QString finalText = text.trimmed();
    if (finalText.isEmpty()) {
        fail(QStringLiteral("Transcription was empty"));
        return;
    }

    if (config_.logging.logTranscripts) qInfo().noquote() << "Transcript:" << finalText;
    else qInfo() << "Transcription complete, characters=" << finalText.size();

    if (!state_.beginInserting()) return;
    tray_->showInserting();
    output_->paste(finalText, config_.output);
}

void DictationController::fail(const QString &message)
{
    maxRecordingTimer_.stop();
    audio_->abort();
    realtime_->abort();
    batch_->abort();
    state_.fail();
    qWarning().noquote() << message;
    tray_->showError(message);
    state_.recover();
}

} // namespace kdict
