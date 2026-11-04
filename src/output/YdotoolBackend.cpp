#include "output/YdotoolBackend.h"

#include "output/PasteKeys.h"
#include "output/YdotoolSocket.h"

#include <QFile>
#include <QFileInfo>
#include <QProcessEnvironment>
#include <QStandardPaths>
#include <QThread>
#include <QTimer>

#include <csignal>
#include <sys/prctl.h>

namespace kdict {
namespace {

QString runtimeDir()
{
    return qEnvironmentVariable("XDG_RUNTIME_DIR");
}

bool socketUsable(const QString &path)
{
    return QFileInfo::exists(path) && YdotoolSocket::probe(path);
}

} // namespace

YdotoolBackend::YdotoolBackend(const OutputConfig &config, QObject *parent)
    : PasteBackend(parent), config_(config)
{
}

YdotoolBackend::~YdotoolBackend()
{
    stopDaemon();
}

QString YdotoolBackend::clientExecutable()
{
    return QStandardPaths::findExecutable(QStringLiteral("ydotool"));
}

QString YdotoolBackend::daemonExecutable()
{
    QString exe = QStandardPaths::findExecutable(QStringLiteral("ydotoold"));
    if (!exe.isEmpty()) return exe;
    // Some distributions install daemons into sbin/libexec, which is not on a
    // normal user's PATH.
    for (const QString &dir : {QStringLiteral("/usr/sbin"), QStringLiteral("/usr/local/sbin"),
                               QStringLiteral("/usr/libexec"), QStringLiteral("/usr/lib/ydotool")}) {
        exe = QStandardPaths::findExecutable(QStringLiteral("ydotoold"), {dir});
        if (!exe.isEmpty()) return exe;
    }
    return {};
}

QString YdotoolBackend::locateSocket() const
{
    const QStringList list = YdotoolSocket::candidates(config_.ydotoolSocket,
                                                       qEnvironmentVariable("YDOTOOL_SOCKET"),
                                                       runtimeDir());
    return YdotoolSocket::firstUsable(list, socketUsable);
}

BackendStatus YdotoolBackend::status()
{
    BackendStatus s;
    const QString client = clientExecutable();
    const QString daemon = daemonExecutable();
    s.details << QStringLiteral("ydotool client: %1").arg(client.isEmpty() ? QStringLiteral("MISSING") : client);
    s.details << QStringLiteral("ydotoold: %1").arg(daemon.isEmpty() ? QStringLiteral("MISSING") : daemon);

    const QString socket = locateSocket();
    if (!socket.isEmpty()) {
        s.details << QStringLiteral("ydotool socket: OK %1").arg(socket);
    } else {
        s.details << QStringLiteral("ydotool socket: none reachable (checked %1)")
                         .arg(YdotoolSocket::candidates(config_.ydotoolSocket,
                                                        qEnvironmentVariable("YDOTOOL_SOCKET"),
                                                        runtimeDir()).join(QStringLiteral(", ")));
    }

    QFileInfo uinput(QStringLiteral("/dev/uinput"));
    const bool uinputWritable = uinput.exists() && QFile(uinput.filePath()).open(QIODevice::WriteOnly);
    s.details << QStringLiteral("/dev/uinput: %1")
                     .arg(!uinput.exists() ? QStringLiteral("MISSING (modprobe uinput)")
                                           : (uinputWritable ? QStringLiteral("writable") : QStringLiteral("not writable by this user")));

    if (client.isEmpty()) {
        s.summary = QStringLiteral("ydotool is not installed");
        s.hint = QStringLiteral("Install ydotool, e.g. ./scripts/install-deps.sh");
        return s;
    }
    if (!socket.isEmpty()) {
        s.ready = true;
        s.summary = QStringLiteral("ydotool via %1").arg(socket);
        return s;
    }
    if (config_.ydotoolAutostart && !daemon.isEmpty() && uinputWritable) {
        s.ready = true;
        s.summary = QStringLiteral("ydotool (ydotoold will be started on demand)");
        return s;
    }
    s.summary = QStringLiteral("ydotoold is not reachable");
    s.hint = uinputWritable
        ? QStringLiteral("Start ydotoold, or set output.ydotool_autostart: true")
        : QStringLiteral("Grant access to /dev/uinput: run ./scripts/setup-uinput.sh, then log out and back in");
    return s;
}

bool YdotoolBackend::startDaemon(QString *error)
{
    const QString daemon = daemonExecutable();
    if (daemon.isEmpty()) {
        if (error) *error = QStringLiteral("ydotoold not found");
        return false;
    }
    const QString runtime = runtimeDir();
    const QString socket = YdotoolSocket::managedPath(runtime);
    if (socket.isEmpty()) {
        if (error) *error = QStringLiteral("XDG_RUNTIME_DIR is not set; cannot create a private ydotool socket");
        return false;
    }

    stopDaemon();
    // A leftover file from a crashed daemon would make the new one fail to bind.
    if (QFileInfo::exists(socket) && !YdotoolSocket::probe(socket)) QFile::remove(socket);

    daemon_ = new QProcess(this);
    daemon_->setProcessChannelMode(QProcess::MergedChannels);
    // The helper must never outlive the application, even after SIGTERM/SIGKILL
    // (session logout, crashes): the kernel signals it when we die.
    daemon_->setChildProcessModifier([]() { ::prctl(PR_SET_PDEATHSIG, SIGTERM); });
    daemon_->start(daemon, {QStringLiteral("--socket-path"), socket});
    if (!daemon_->waitForStarted(1000)) {
        if (error) *error = QStringLiteral("Could not start ydotoold: %1").arg(daemon_->errorString());
        stopDaemon();
        return false;
    }

    // ydotoold creates the socket right after opening /dev/uinput.
    for (int i = 0; i < 80; ++i) {
        if (daemon_->state() == QProcess::NotRunning) break;
        if (socketUsable(socket)) {
            // The compositor needs a moment to register the new virtual
            // keyboard; keys sent immediately after creation are dropped.
            QThread::msleep(400);
            return true;
        }
        QThread::msleep(25);
    }

    const QString output = QString::fromUtf8(daemon_->readAll()).trimmed();
    if (error) {
        *error = daemon_->state() == QProcess::NotRunning
            ? QStringLiteral("ydotoold exited immediately%1 (is /dev/uinput writable? run ./scripts/setup-uinput.sh)")
                  .arg(output.isEmpty() ? QString() : QStringLiteral(": %1").arg(output))
            : QStringLiteral("ydotoold did not create %1 in time").arg(socket);
    }
    stopDaemon();
    return false;
}

void YdotoolBackend::stopDaemon()
{
    if (!daemon_) return;
    QProcess *daemon = daemon_;
    daemon_ = nullptr;
    if (daemon->state() != QProcess::NotRunning) {
        daemon->terminate();
        if (!daemon->waitForFinished(500)) {
            daemon->kill();
            daemon->waitForFinished(500);
        }
    }
    delete daemon;
}

bool YdotoolBackend::prepare(QString *error)
{
    if (clientExecutable().isEmpty()) {
        if (error) *error = QStringLiteral("ydotool not found");
        return false;
    }
    socket_ = locateSocket();
    if (!socket_.isEmpty()) return true;
    if (!config_.ydotoolAutostart) {
        if (error) *error = QStringLiteral("ydotoold socket not found and output.ydotool_autostart is disabled");
        return false;
    }
    if (!startDaemon(error)) return false;
    socket_ = locateSocket();
    return !socket_.isEmpty();
}

void YdotoolBackend::sendPaste(const QString &shortcut, Done done)
{
    // A previously valid socket can disappear (daemon stopped, session change).
    if (socket_.isEmpty() || !socketUsable(socket_)) {
        QString error;
        if (!prepare(&error)) {
            QTimer::singleShot(0, this, [done = std::move(done), error]() { done(false, error); });
            return;
        }
    }

    auto *process = new QProcess(this);
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert(QStringLiteral("YDOTOOL_SOCKET"), socket_);
    process->setProcessEnvironment(env);

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
    process->start(clientExecutable(), PasteKeys::ydotoolArgs(shortcut));
}

} // namespace kdict
