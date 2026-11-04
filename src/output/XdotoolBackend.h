#pragma once

#include "output/PasteBackend.h"

namespace kdict {

// X11 sessions only: no uinput permissions or daemon required.
class XdotoolBackend : public PasteBackend {
    Q_OBJECT
public:
    explicit XdotoolBackend(QObject *parent = nullptr) : PasteBackend(parent) {}

    QString name() const override { return QStringLiteral("xdotool"); }
    BackendStatus status() override;
    bool prepare(QString *error) override;
    void sendPaste(const QString &shortcut, Done done) override;

    static QString executable();
};

} // namespace kdict
