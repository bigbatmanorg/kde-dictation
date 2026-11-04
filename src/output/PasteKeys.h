#pragma once

#include <QString>
#include <QStringList>

namespace kdict {

// Pure helpers that translate the configured paste shortcut into the argument
// vectors understood by the key-injection tools. Kept free of QProcess so they
// can be unit-tested on any machine.
class PasteKeys {
public:
    // True for the shortcuts the application supports ("ctrl+v", "shift+insert").
    static bool isSupported(const QString &shortcut);

    // ydotool >= 1.0 raw key events: "key 29:1 47:1 47:0 29:0".
    static QStringList ydotoolArgs(const QString &shortcut);

    // xdotool key names: "key --clearmodifiers ctrl+v".
    static QStringList xdotoolArgs(const QString &shortcut);
};

} // namespace kdict
