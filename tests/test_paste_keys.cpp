#include <QtTest>
#include "output/PasteKeys.h"

using namespace kdict;

class PasteKeysTest : public QObject {
    Q_OBJECT
private slots:
    void supportedShortcuts()
    {
        QVERIFY(PasteKeys::isSupported(QStringLiteral("ctrl+v")));
        QVERIFY(PasteKeys::isSupported(QStringLiteral(" Shift+Insert ")));
        QVERIFY(!PasteKeys::isSupported(QStringLiteral("ctrl+shift+v")));
    }
    void ydotoolCtrlV()
    {
        const QStringList expected{"key", "29:1", "47:1", "47:0", "29:0"};
        QCOMPARE(PasteKeys::ydotoolArgs(QStringLiteral("ctrl+v")), expected);
    }
    void ydotoolShiftInsert()
    {
        const QStringList expected{"key", "42:1", "110:1", "110:0", "42:0"};
        QCOMPARE(PasteKeys::ydotoolArgs(QStringLiteral("SHIFT+INSERT")), expected);
    }
    void xdotool()
    {
        QCOMPARE(PasteKeys::xdotoolArgs(QStringLiteral("ctrl+v")).last(), QStringLiteral("ctrl+v"));
        QCOMPARE(PasteKeys::xdotoolArgs(QStringLiteral("shift+insert")).last(), QStringLiteral("shift+Insert"));
        QVERIFY(PasteKeys::xdotoolArgs(QStringLiteral("ctrl+v")).contains(QStringLiteral("--clearmodifiers")));
    }
};

QTEST_MAIN(PasteKeysTest)
#include "test_paste_keys.moc"
