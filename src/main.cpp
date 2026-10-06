#include "app/DictationController.h"
#include "audio/AudioRecorder.h"
#include "config/Config.h"
#include "output/TextOutput.h"
#include "shortcut/ShortcutController.h"
#include "stt/BatchOpenAIProvider.h"
#include "stt/RealtimeAsrProvider.h"
#include "ui/TrayController.h"
#include "util/UrlHelpers.h"

#include <QApplication>
#include <QAudioDevice>
#include <QAudioFormat>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QFile>
#include <QFileInfo>
#include <QLockFile>
#include <QMediaDevices>
#include <QStandardPaths>
#include <QTextStream>
#include <QTimer>

#include <iostream>
#include <memory>

using namespace kdict;

namespace {

int runCheck(const QString &configPath)
{
    QTextStream out(stdout);
    try {
        const auto cfg = ConfigLoader::load(configPath);
        out << "config: OK (" << configPath << ")\n";
        out << "activation: " << toString(cfg.activationMode) << "\n";
        out << "transcription: " << toString(cfg.transcription.mode) << "\n";
        out << "batch endpoint: " << makeHttpEndpoint(cfg.transcription.baseUrl, cfg.transcription.batch.path).toString() << "\n";
        out << "realtime endpoint: " << makeWebSocketEndpoint(cfg.transcription.baseUrl, cfg.transcription.realtime.path).toString() << "\n";
        out << "api key: " << (cfg.transcription.resolvedApiKey().isEmpty() ? "MISSING" : "present") << "\n";

        const QString ydotool = TextOutput::ydotoolExecutable();
        out << "ydotool: " << (ydotool.isEmpty() ? "MISSING" : ydotool) << "\n";
        const QString socket = TextOutput::ydotoolSocketPath();
        out << "ydotool socket: " << (!socket.isEmpty() && QFileInfo::exists(socket) ? "OK " : "MISSING ") << socket << "\n";

        const QAudioDevice device = QMediaDevices::defaultAudioInput();
        if (device.isNull()) {
            out << "microphone: MISSING\n";
            return 2;
        }
        QAudioFormat fmt;
        fmt.setSampleRate(cfg.audio.sampleRate);
        fmt.setChannelCount(1);
        fmt.setSampleFormat(QAudioFormat::Int16);
        out << "microphone: " << device.description() << "\n";
        out << "PCM16 mono " << cfg.audio.sampleRate << " Hz: " << (device.isFormatSupported(fmt) ? "supported" : "NOT SUPPORTED") << "\n";

        return (ydotool.isEmpty() || socket.isEmpty() || !QFileInfo::exists(socket) || cfg.transcription.resolvedApiKey().isEmpty()) ? 2 : 0;
    } catch (const std::exception &e) {
        out << "config: ERROR: " << e.what() << "\n";
        return 2;
    }
}

} // namespace

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    QApplication::setQuitOnLastWindowClosed(false);
    QCoreApplication::setApplicationName(QStringLiteral("org.amvc.kdedictation"));
    QCoreApplication::setApplicationVersion(QStringLiteral(KDICT_VERSION));
    QGuiApplication::setApplicationDisplayName(QStringLiteral("KDE Dictation"));
    QGuiApplication::setDesktopFileName(QStringLiteral("org.amvc.kdedictation"));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("KDE Plasma global shortcut dictation client"));
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption configOpt({QStringLiteral("c"), QStringLiteral("config")},
                                 QStringLiteral("Configuration file"), QStringLiteral("path"), ConfigLoader::defaultPath());
    QCommandLineOption checkOpt(QStringLiteral("check"), QStringLiteral("Check local configuration, audio and ydotool prerequisites"));
    QCommandLineOption outputOpt(QStringLiteral("test-output"), QStringLiteral("Copy and paste static text after a delay"), QStringLiteral("text"));
    QCommandLineOption delayOpt(QStringLiteral("delay-ms"), QStringLiteral("Delay used by --test-output"), QStringLiteral("ms"), QStringLiteral("3000"));
    QCommandLineOption sttOpt(QStringLiteral("test-stt"), QStringLiteral("Send an existing WAV through the configured batch STT endpoint"), QStringLiteral("wav"));
    parser.addOption(configOpt);
    parser.addOption(checkOpt);
    parser.addOption(outputOpt);
    parser.addOption(delayOpt);
    parser.addOption(sttOpt);
    parser.process(app);

    const QString configPath = parser.value(configOpt);

    if (parser.isSet(checkOpt)) return runCheck(configPath);

    AppConfig config;
    try {
        config = ConfigLoader::load(configPath);
    } catch (const std::exception &e) {
        std::cerr << "Configuration error: " << e.what() << '\n';
        return 2;
    }

    if (parser.isSet(outputOpt)) {
        TextOutput output;
        QObject::connect(&output, &TextOutput::warning, [](const QString &m) { qWarning().noquote() << m; });
        QObject::connect(&output, &TextOutput::finished, &app, &QCoreApplication::quit);
        bool ok = false;
        const int delay = parser.value(delayOpt).toInt(&ok);
        QTimer::singleShot(ok ? qMax(0, delay) : 3000, [&output, &config, &parser]() {
            output.paste(parser.value(QStringLiteral("test-output")), config.output);
        });
        return app.exec();
    }

    if (parser.isSet(sttOpt)) {
        QFile f(parser.value(sttOpt));
        if (!f.open(QIODevice::ReadOnly)) {
            qCritical().noquote() << "Cannot open WAV:" << f.fileName();
            return 2;
        }
        const QByteArray wav = f.readAll();
        BatchOpenAIProvider batch;
        QObject::connect(&batch, &BatchOpenAIProvider::finalTranscript, &app, [&](const QString &text) {
            QTextStream(stdout) << text << '\n';
            app.exit(0);
        });
        QObject::connect(&batch, &BatchOpenAIProvider::error, &app, [&](const QString &message) {
            qCritical().noquote() << message;
            app.exit(2);
        });
        QTimer::singleShot(0, [&]() { batch.transcribe(wav, config.transcription); });
        return app.exec();
    }

    const QString runtime = QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation);
    QLockFile lock(runtime + QStringLiteral("/kde-dictation.lock"));
    lock.setStaleLockTime(0);
    if (!lock.tryLock()) {
        qCritical() << "KDE Dictation is already running";
        return 2;
    }

    AudioRecorder audio;
    BatchOpenAIProvider batch;
    RealtimeAsrProvider realtime;
    TextOutput output;
    ShortcutController shortcut;
    TrayController tray;
    DictationController controller(configPath, &audio, &batch, &realtime, &output, &shortcut, &tray);

    QObject::connect(&tray, &TrayController::quitRequested, &app, &QCoreApplication::quit);

    QString initError;
    if (!controller.initialize(&initError)) {
        qCritical().noquote() << initError;
        tray.showError(initError);
    }

    return app.exec();
}
