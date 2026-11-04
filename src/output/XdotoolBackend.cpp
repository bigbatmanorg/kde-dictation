#include "output/XdotoolBackend.h"

#include "output/PasteKeys.h"
#include "util/Session.h"

#include <QProcess>
#include <QStandardPaths>
#include <QTimer>

namespace kdict {

QString XdotoolBackend::executable()
{
    return QStandardPaths::findExecutable(QStringLiteral("xdotool"));
}

BackendStatus XdotoolBackend::status()
{
    BackendStatus s;
    const QString exe = executable();
    const SessionType session = Session::detect();
    s.details << QStringLiteral("xdotool: %1").arg(exe.isEmpty() ? QStringLiteral("MISSING") : exe);
    s.details << QStringLiteral("session: %1").arg(toString(session));
    if (exe.isEmpty()) {
        s.summary = QStringLiteral("xdotool is not installed");
        s.hint = QStringLiteral("Install xdotool, or use the ydotool backend");
    } else if (session == SessionType::Wayland) {
        s.summary = QStringLiteral("xdotool cannot send keys to native Wayland windows");
        s.hint = QStringLiteral("Set output.backend: ydotool on Wayland sessions");
    } else {
        s.ready = true;
        s.summary = QStringLiteral("xdotool");
    }
    return s;
}

bool XdotoolBackend::prepare(QString *error)
{
    const BackendStatus s = status();
    if (!s.ready && error) *error = s.summary;
    return s.ready;
}

void XdotoolBackend::sendPaste(const QString &shortcut, Done done)
{
    const QString exe = executable();
    if (exe.isEmpty()) {
        QTimer::singleShot(0, this, [done = std::move(done)]() { done(false, QStringLiteral("xdotool not found")); });
        return;
    }
    auto *process = new QProcess(this);
    connect(process, &QProcess::finished, this,
            [process, done](int exitCode, QProcess::ExitStatus status) {
        const QString stderrText = QString::fromUtf8(process->readAllStandardError()).trimmed();
        process->deleteLater();
        const bool ok = status == QProcess::NormalExit && exitCode == 0;
        done(ok, ok ? QString() : stderrText);
    });
    connect(process, &QProcess::errorOccurred, this, [this, process, done](QProcess::ProcessError e) {
        if (e != QProcess::FailedToStart) return;
        const QString reason = process->errorString();
        process->disconnect(this);
        process->deleteLater();
        done(false, reason);
    });
    process->start(exe, PasteKeys::xdotoolArgs(shortcut));
}

} // namespace kdict
