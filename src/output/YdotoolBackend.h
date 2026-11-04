#pragma once

#include "output/PasteBackend.h"

#include <QProcess>

namespace kdict {

// Works on Wayland and X11 through /dev/uinput. If no ydotoold is reachable the
// backend starts a private one (own socket under $XDG_RUNTIME_DIR), so the user
// does not need to enable a distro-specific systemd unit.
class YdotoolBackend : public PasteBackend {
    Q_OBJECT
public:
    explicit YdotoolBackend(const OutputConfig &config, QObject *parent = nullptr);
    ~YdotoolBackend() override;

    QString name() const override { return QStringLiteral("ydotool"); }
    BackendStatus status() override;
    bool prepare(QString *error) override;
    void sendPaste(const QString &shortcut, Done done) override;

    static QString clientExecutable();
    static QString daemonExecutable();

private:
    QString locateSocket() const;
    bool startDaemon(QString *error);
    void stopDaemon();

    OutputConfig config_;
    QString socket_;
    QProcess *daemon_ = nullptr;
};

} // namespace kdict
