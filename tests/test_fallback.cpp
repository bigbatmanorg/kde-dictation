#include <QtTest>
#include "app/FallbackPolicy.h"

using namespace kdict;

class FallbackTest : public QObject {
    Q_OBJECT
private slots:
    void recordingFailureDefers()
    {
        QCOMPARE(FallbackPolicy::onRealtimeFailure(DictationState::Recording, true), FallbackAction::DeferUntilStop);
    }

    void finalizationFailureStartsBatch()
    {
        QCOMPARE(FallbackPolicy::onRealtimeFailure(DictationState::Finalizing, true), FallbackAction::StartBatchNow);
    }

    void disabledMeansNoFallback()
    {
        QCOMPARE(FallbackPolicy::onRealtimeFailure(DictationState::Finalizing, false), FallbackAction::None);
    }
};

QTEST_MAIN(FallbackTest)
#include "test_fallback.moc"
