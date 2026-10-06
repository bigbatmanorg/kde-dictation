#pragma once

#include "config/Config.h"
#include <QObject>
#include <QStringList>

namespace kdict {

class TextOutput : public QObject {
    Q_OBJECT
public:
    explicit TextOutput(QObject *parent = nullptr);

    void paste(const QString &text, const OutputConfig &config);
    static QString ydotoolExecutable();
    static QString ydotoolSocketPath();

signals:
    void finished();
    void warning(const QString &message);

private:
    static QStringList ydotoolArgs(const QString &shortcut);
    static bool synchronizeKdeClipboard(const QString &text);
};

} // namespace kdict
