#include "output/PasteKeys.h"

namespace kdict {
namespace {

bool isShiftInsert(const QString &shortcut)
{
    return shortcut.trimmed().toLower() == QStringLiteral("shift+insert");
}

} // namespace

bool PasteKeys::isSupported(const QString &shortcut)
{
    const QString normalized = shortcut.trimmed().toLower();
    return normalized == QStringLiteral("ctrl+v") || normalized == QStringLiteral("shift+insert");
}

QStringList PasteKeys::ydotoolArgs(const QString &shortcut)
{
    if (isShiftInsert(shortcut)) {
        // KEY_LEFTSHIFT=42, KEY_INSERT=110.
        return {QStringLiteral("key"), QStringLiteral("42:1"), QStringLiteral("110:1"),
                QStringLiteral("110:0"), QStringLiteral("42:0")};
    }
    // KEY_LEFTCTRL=29, KEY_V=47. Raw keycodes are layout independent.
    return {QStringLiteral("key"), QStringLiteral("29:1"), QStringLiteral("47:1"),
            QStringLiteral("47:0"), QStringLiteral("29:0")};
}

QStringList PasteKeys::xdotoolArgs(const QString &shortcut)
{
    return {QStringLiteral("key"), QStringLiteral("--clearmodifiers"),
            isShiftInsert(shortcut) ? QStringLiteral("shift+Insert") : QStringLiteral("ctrl+v")};
}

} // namespace kdict
