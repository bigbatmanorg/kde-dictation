#include "app/DictationController.h"
#include "audio/AudioRecorder.h"
#include "config/Config.h"
#include "output/PasteBackend.h"
#include "output/TextOutput.h"
#include "shortcut/ShortcutController.h"
#include "stt/BatchOpenAIProvider.h"
#include "stt/RealtimeAsrProvider.h"
#include "ui/TrayController.h"
#include "util/Session.h"
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
        out << "realtime live insert: " << (cfg.transcription.realtime.liveInsert ? "enabled" : "disabled")
            << " (flush " << cfg.transcription.realtime.liveFlushMs << " ms)\n";
        out << "api key: " << (cfg.transcription.resolvedApiKey().isEmpty() ? "MISSING" : "present") << "\n";

        out << "session: " << toString(Session::detect()) << "\n";
        const BackendStatus paste = TextOutput::backendStatus(cfg.output);
        out << "paste backend: " << PasteBackendFactory::resolveName(cfg.output.backend)
            << " (" << (paste.ready ? "ready" : "NOT READY") << ") - " << paste.summary << "\n";
        for (const QString &line : paste.details) out << "  " << line << "\n";
        if (!paste.ready && !paste.hint.isEmpty()) out << "  hint: " << paste.hint << "\n";

        const QAudioDevice device = AudioRecorder::resolveDevice(cfg.audio.device);
        if (device.isNull()) {
            out << "microphone: MISSING (audio.device=" << cfg.audio.device << ")\n";
            return 2;
        }
        QAudioFormat fmt;
        fmt.setSampleRate(cfg.audio.sampleRate);
        fmt.setChannelCount(1);
        fmt.setSampleFormat(QAudioFormat::Int16);
        out << "microphone: " << device.description() << "\n";
        out << "PCM16 mono " << cfg.audio.sampleRate << " Hz: " << (device.isFormatSupported(fmt) ? "supported" : "NOT SUPPORTED") << "\n";

        return (!paste.ready || cfg.transcription.resolvedApiKey().isEmpty()) ? 2 : 0;
    } catch (const std::exception &e) {
        out << "config: ERROR: " << e.what() << "\n";
        return 2;
    }
}

struct CliOptions {
    QCommandLineOption config{{QStringLiteral("c"), QStringLiteral("config")},
                              QStringLiteral("Configuration file"), QStringLiteral("path"), ConfigLoader::defaultPath()};
    QCommandLineOption check{QStringLiteral("check"), QStringLiteral("Check local configuration, audio and paste-backend prerequisites")};
    QCommandLineOption output{QStringLiteral("test-output"), QStringLiteral("Copy and paste static text after a delay"), QStringLiteral("text")};
    QCommandLineOption delay{QStringLiteral("delay-ms"), QStringLiteral("Delay used by --test-output"), QStringLiteral("ms"), QStringLiteral("3000")};
    QCommandLineOption stt{QStringLiteral("test-stt"), QStringLiteral("Send an existing WAV through the configured batch STT endpoint"), QStringLiteral("wav")};
    QCommandLineOption devices{QStringLiteral("list-devices"), QStringLiteral("List audio input devices usable for audio.device")};

    void install(QCommandLineParser &parser)
    {
        parser.setApplicationDescription(QStringLiteral("KDE Plasma global shortcut dictation client"));
        parser.addHelpOption();
        parser.addVersionOption();
        parser.addOption(devices);
        parser.addOption(config);
        parser.addOption(check);
        parser.addOption(output);
        parser.addOption(delay);
        parser.addOption(stt);
    }
};

void setApplicationIdentity()
{
    QCoreApplication::setApplicationName(QStringLiteral("org.amvc.kdedictation"));
    QCoreApplication::setApplicationVersion(QStringLiteral(KDICT_VERSION));
}

bool wantsInfoOnly(int argc, char **argv)
{
    for (int i = 1; i < argc; ++i) {
        const QString arg = QString::fromLocal8Bit(argv[i]);
        if (arg == QLatin1String("-h") || arg == QLatin1String("--help") || arg == QLatin1String("--help-all")
            || arg == QLatin1String("-v") || arg == QLatin1String("--version")) {
            return true;
        }
    }
    return false;
}

} // namespace

int main(int argc, char **argv)
{
    // --help/--version must work without a display (SSH, CI, package builds).
    if (wantsInfoOnly(argc, argv)) {
        QCoreApplication core(argc, argv);
        setApplicationIdentity();
        QCommandLineParser parser;
        CliOptions options;
        options.install(parser);
        parser.process(core); // prints and exits
        return 0;
    }

    QApplication app(argc, argv);
    QApplication::setQuitOnLastWindowClosed(false);
    setApplicationIdentity();
    QGuiApplication::setApplicationDisplayName(QStringLiteral("KDE Dictation"));
    QGuiApplication::setDesktopFileName(QStringLiteral("org.amvc.kdedictation"));

    QCommandLineParser parser;
    CliOptions options;
    options.install(parser);
    parser.process(app);

    const QString configPath = parser.value(options.config);

    if (parser.isSet(options.devices)) {
        QTextStream out(stdout);
        const QStringList inputs = AudioRecorder::describeInputs();
        out << (inputs.isEmpty() ? QStringLiteral("No audio input devices found\n") : inputs.join(QLatin1Char('\n')) + QLatin1Char('\n'));
        out << "(* = system default; use a description fragment or id as audio.device)\n";
        return inputs.isEmpty() ? 2 : 0;
    }

    if (parser.isSet(options.check)) return runCheck(configPath);

    AppConfig config;
    try {
        config = ConfigLoader::load(configPath);
    } catch (const std::exception &e) {
        std::cerr << "Configuration error: " << e.what() << '\n';
        return 2;
    }

    if (parser.isSet(options.output)) {
        TextOutput output;
        QObject::connect(&output, &TextOutput::warning, [](const QString &m) { qWarning().noquote() << m; });
        // A self-managed ydotoold dies with this process; give it time to inject the keys.
        QObject::connect(&output, &TextOutput::finished, &app, []() { QTimer::singleShot(400, &QCoreApplication::quit); });
        bool ok = false;
        const int delay = parser.value(options.delay).toInt(&ok);
        output.warmUp(config.output);
        QTimer::singleShot(ok ? qMax(0, delay) : 3000, [&output, &config, &parser]() {
            output.paste(parser.value(QStringLiteral("test-output")), config.output);
        });
        return app.exec();
    }

    if (parser.isSet(options.stt)) {
        QFile f(parser.value(options.stt));
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
