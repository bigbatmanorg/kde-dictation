#include <QtTest>
#include "app/DictationState.h"

using namespace kdict;

class StateTest : public QObject {
    Q_OBJECT
private slots:
    void happyPath()
    {
        DictationStateMachine s;
        QCOMPARE(s.state(), DictationState::Idle);
        QVERIFY(s.beginRecording());
        QCOMPARE(s.state(), DictationState::Recording);
        QVERIFY(s.beginFinalizing());
        QCOMPARE(s.state(), DictationState::Finalizing);
        QVERIFY(s.beginInserting());
        QCOMPARE(s.state(), DictationState::Inserting);
        s.complete();
        QCOMPARE(s.state(), DictationState::Idle);
    }

    void rejectsInvalidTransitions()
    {
        DictationStateMachine s;
        QVERIFY(!s.beginFinalizing());
        QVERIFY(!s.beginInserting());
        QVERIFY(s.beginRecording());
        QVERIFY(!s.beginRecording());
        QVERIFY(!s.beginInserting());
    }

    void errorRecovery()
    {
        DictationStateMachine s;
        QVERIFY(s.beginRecording());
        s.fail();
        QCOMPARE(s.state(), DictationState::Error);
        s.recover();
        QCOMPARE(s.state(), DictationState::Idle);
    }
};

QTEST_MAIN(StateTest)
#include "test_state.moc"
