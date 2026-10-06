#pragma once

#include "config/Config.h"
#include <QObject>

class QAction;

namespace kdict {

class ShortcutController : public QObject {
    Q_OBJECT
public:
    explicit ShortcutController(QObject *parent = nullptr);

    void setMode(ActivationMode mode) { mode_ = mode; }
    ActivationMode mode() const { return mode_; }

signals:
    void startRequested();
    void stopRequested();
    void toggleRequested();

private:
    QAction *action_ = nullptr;
    ActivationMode mode_ = ActivationMode::Toggle;
};

} // namespace kdict
