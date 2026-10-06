#include <QtTest>
#include "stt/RealtimeEventParser.h"

using namespace kdict;

class RealtimeParserTest : public QObject {
    Q_OBJECT
private slots:
    void parsesLifecycle()
    {
        QCOMPARE(RealtimeEventParser::parse(R"({"type":"session.created"})").kind, RealtimeEventKind::SessionCreated);
        QCOMPARE(RealtimeEventParser::parse(R"({"type":"session.updated"})").kind, RealtimeEventKind::SessionUpdated);
        QCOMPARE(RealtimeEventParser::parse(R"({"type":"input_audio_buffer.committed"})").kind, RealtimeEventKind::Committed);
    }

    void parsesDeltaAndCompleted()
    {
        auto delta = RealtimeEventParser::parse(R"({"type":"conversation.item.input_audio_transcription.delta","delta":"hello"})");
        QCOMPARE(delta.kind, RealtimeEventKind::Delta);
        QCOMPARE(delta.text, QStringLiteral("hello"));

        auto completed = RealtimeEventParser::parse(R"({"type":"conversation.item.input_audio_transcription.completed","transcript":"hello world"})");
        QCOMPARE(completed.kind, RealtimeEventKind::Completed);
        QCOMPARE(completed.text, QStringLiteral("hello world"));
    }

    void parsesError()
    {
        auto event = RealtimeEventParser::parse(R"({"type":"error","error":{"message":"boom"}})");
        QCOMPARE(event.kind, RealtimeEventKind::Error);
        QCOMPARE(event.errorMessage, QStringLiteral("boom"));
    }
};

QTEST_MAIN(RealtimeParserTest)
#include "test_realtime_parser.moc"
