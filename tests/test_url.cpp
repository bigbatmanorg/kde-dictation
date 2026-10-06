#include <QtTest>
#include "util/UrlHelpers.h"

using namespace kdict;

class UrlTest : public QObject {
    Q_OBJECT
private slots:
    void normalizesHttpAndWebsocket()
    {
        QCOMPARE(makeHttpEndpoint(QUrl("https://ai.example/v1/"), "/v1/audio/transcriptions").toString(),
                 QStringLiteral("https://ai.example/v1/audio/transcriptions"));
        QCOMPARE(makeWebSocketEndpoint(QUrl("https://ai.example/"), "/v1/audio/transcriptions/realtime").toString(),
                 QStringLiteral("wss://ai.example/v1/audio/transcriptions/realtime"));
    }
};

QTEST_MAIN(UrlTest)
#include "test_url.moc"
