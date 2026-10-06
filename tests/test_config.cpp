#include <QtTest>
#include <QFile>
#include <QTemporaryDir>
#include "config/Config.h"

using namespace kdict;

class ConfigTest : public QObject {
    Q_OBJECT
private slots:
    void parsesAndResolvesEnvKey()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString path = dir.filePath(QStringLiteral("config.yaml"));
        QFile f(path);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write(R"YAML(
activation:
  mode: push_to_talk
audio:
  sample_rate: 16000
  max_recording_seconds: 120
transcription:
  mode: realtime
  base_url: https://example.test
  api_key_env: TEST_KDICT_KEY
  api_key: fallback
  batch:
    model: whisper-1
  realtime:
    fallback_to_batch: true
    finalization_timeout_ms: 2500
    live_insert: true
    live_flush_ms: 90
output:
  method: clipboard_paste
  paste_shortcut: ctrl+v
  live_paste_delay_ms: 35
)YAML");
        f.close();

        qputenv("TEST_KDICT_KEY", "from-env");
        const auto cfg = ConfigLoader::load(path);
        QCOMPARE(static_cast<int>(cfg.activationMode), static_cast<int>(ActivationMode::PushToTalk));
        QCOMPARE(static_cast<int>(cfg.transcription.mode), static_cast<int>(TranscriptionMode::Realtime));
        QCOMPARE(cfg.transcription.resolvedApiKey(), QStringLiteral("from-env"));
        QCOMPARE(cfg.transcription.realtime.finalizationTimeoutMs, 2500);
        QVERIFY(cfg.transcription.realtime.fallbackToBatch);
        QVERIFY(cfg.transcription.realtime.liveInsert);
        QCOMPARE(cfg.transcription.realtime.liveFlushMs, 90);
        QCOMPARE(cfg.output.livePasteDelayMs, 35);
        qunsetenv("TEST_KDICT_KEY");
    }

    void rejectsBadPasteShortcut()
    {
        QTemporaryDir dir;
        const QString path = dir.filePath(QStringLiteral("config.yaml"));
        QFile f(path);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write(R"YAML(
transcription:
  base_url: https://example.test
output:
  method: clipboard_paste
  paste_shortcut: ctrl+shift+v
)YAML");
        f.close();
        QVERIFY_EXCEPTION_THROWN(ConfigLoader::load(path), std::runtime_error);
    }
};

QTEST_MAIN(ConfigTest)
#include "test_config.moc"
