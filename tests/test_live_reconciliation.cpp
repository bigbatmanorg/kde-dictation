#include <QtTest>
#include "app/LiveTranscriptReconciler.h"

using namespace kdict;

class LiveReconciliationTest : public QObject {
    Q_OBJECT
private slots:
    void exact()
    {
        const auto r = LiveTranscriptReconciler::reconcile(QStringLiteral("hello world"), QStringLiteral("hello world"));
        QCOMPARE(r.kind, LiveReconcileKind::Exact);
        QVERIFY(r.suffix.isEmpty());
    }

    void appendsOnlyMissingSuffix()
    {
        const auto r = LiveTranscriptReconciler::reconcile(QStringLiteral("hello"), QStringLiteral("hello world"));
        QCOMPARE(r.kind, LiveReconcileKind::AppendSuffix);
        QCOMPARE(r.suffix, QStringLiteral(" world"));
    }

    void toleratesWhitespaceOnlyDifference()
    {
        const auto r = LiveTranscriptReconciler::reconcile(QStringLiteral("hello world "), QStringLiteral("hello world"));
        QCOMPARE(r.kind, LiveReconcileKind::EquivalentWhitespace);
    }

    void detectsLongerStream()
    {
        const auto r = LiveTranscriptReconciler::reconcile(QStringLiteral("hello world again"), QStringLiteral("hello world"));
        QCOMPARE(r.kind, LiveReconcileKind::StreamedLonger);
    }

    void detectsMismatch()
    {
        const auto r = LiveTranscriptReconciler::reconcile(QStringLiteral("hello there"), QStringLiteral("hello world"));
        QCOMPARE(r.kind, LiveReconcileKind::Mismatch);
    }
};

QTEST_MAIN(LiveReconciliationTest)
#include "test_live_reconciliation.moc"
