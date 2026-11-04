#pragma once

#include "config/Config.h"

#include <QObject>
#include <QString>
#include <QStringList>
#include <functional>
#include <memory>

namespace kdict {

struct BackendStatus {
    bool ready = false;
    QString summary;          // one line, e.g. "ydotool 1.0.4 via /run/user/1000/..sock"
    QStringList details;      // extra diagnostic lines for --check
    QString hint;             // actionable fix when !ready
};

// Injects the paste keystroke into whatever owns keyboard focus. The clipboard
// itself is handled by TextOutput; backends only synthesize the shortcut.
class PasteBackend : public QObject {
    Q_OBJECT
public:
    using Done = std::function<void(bool ok, const QString &error)>;

    explicit PasteBackend(QObject *parent = nullptr) : QObject(parent) {}

    virtual QString name() const = 0;

    // Pure inspection: never starts processes.
    virtual BackendStatus status() = 0;

    // Make the backend usable (e.g. start a helper daemon). Returns false and
    // fills `error` when that is not possible.
    virtual bool prepare(QString *error) = 0;

    virtual void sendPaste(const QString &shortcut, Done done) = 0;
};

class PasteBackendFactory {
public:
    // Resolves "auto|ydotool|xdotool" against the running session.
    static std::unique_ptr<PasteBackend> create(const OutputConfig &config, QObject *parent = nullptr);

    // Name chosen for the given configuration (without constructing it).
    static QString resolveName(const QString &configured);
};

} // namespace kdict
