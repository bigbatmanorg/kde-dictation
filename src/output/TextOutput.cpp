#include "output/TextOutput.h"

#include <QClipboard>
#include <QDBusInterface>
#include <QDBusReply>
#include <QFileInfo>
#include <QGuiApplication>
#include <QProcess>
#include <QStandardPaths>
#include <QTimer>

namespace kdict {

TextOutput::TextOutput(QObject *parent) : QObject(parent) {}

QString TextOutput::ydotoolExecutable()
{
    return QStandardPaths::findExecutable(QStringLiteral("ydotool"));
}

QString TextOutput::ydotoolSocketPath()
{
    const QString runtime = qEnvironmentVariable("XDG_RUNTIME_DIR");
    if (runtime.isEmpty()) return {};
    return runtime + QStringLiteral("/.ydotool_socket");
}

QStringList TextOutput::ydotoolArgs(const QString &shortcut)
{
    const QString normalized = shortcut.trimmed().toLower();
    if (normalized == QStringLiteral("shift+insert")) {
        return {QStringLiteral("key"), QStringLiteral("42:1"), QStringLiteral("110:1"), QStringLiteral("110:0"), QStringLiteral("42:0")};
    }
    // Linux input key codes: KEY_LEFTCTRL=29, KEY_V=47.
    return {QStringLiteral("key"), QStringLiteral("29:1"), QStringLiteral("47:1"), QStringLiteral("47:0"), QStringLiteral("29:0")};
}

bool TextOutput::synchronizeKdeClipboard(const QString &text)
{
    // Qt is the normal clipboard path.  On some Plasma Wayland sessions Klipper
    // can still hold a stale offer when the paste shortcut is sent.  Check its
    // public interface only at insertion time and repair that stale offer.
    QDBusInterface klipper(QStringLiteral("org.kde.klipper"),
                            QStringLiteral("/klipper"),
                            QStringLiteral("org.kde.klipper.klipper"));
    if (!klipper.isValid()) return false;

    const QDBusReply<QString> current = klipper.call(QStringLiteral("getClipboardContents"));
    if (!current.isValid() || current.value() == text) return false;

    const QDBusMessage reply = klipper.call(QStringLiteral("setClipboardContents"), text);
    return reply.type() != QDBusMessage::ErrorMessage;
}

void TextOutput::paste(const QString &text, const OutputConfig &config)
{
    QGuiApplication::clipboard()->setText(text, QClipboard::Clipboard);

    const QString executable = ydotoolExecutable();
    if (executable.isEmpty()) {
        emit warning(QStringLiteral("ydotool not found; transcript was copied to the clipboard"));
        emit finished();
        return;
    }

    const QString socket = ydotoolSocketPath();
    if (socket.isEmpty() || !QFileInfo::exists(socket)) {
        emit warning(QStringLiteral("ydotoold socket not found; transcript was copied to the clipboard"));
        emit finished();
        return;
    }

    const auto startPaste = [this, executable, config]() {
        auto *process = new QProcess(this);
        connect(process, &QProcess::finished, this, [this, process](int exitCode, QProcess::ExitStatus status) {
            const QString stderrText = QString::fromUtf8(process->readAllStandardError()).trimmed();
            process->deleteLater();
            if (status != QProcess::NormalExit || exitCode != 0) {
                emit warning(QStringLiteral("ydotool paste failed; transcript remains in clipboard%1")
                                 .arg(stderrText.isEmpty() ? QString() : QStringLiteral(": %1").arg(stderrText)));
            }
            emit finished();
        });
        process->start(executable, ydotoolArgs(config.pasteShortcut));
        if (!process->waitForStarted(500)) {
            const QString reason = process->errorString();
            QObject::disconnect(process, nullptr, this, nullptr);
            process->deleteLater();
            emit warning(QStringLiteral("Could not start ydotool; transcript remains in clipboard: %1").arg(reason));
            emit finished();
            return;
        }
    };

    QTimer::singleShot(config.pasteDelayMs, this, [this, text, startPaste]() {
        if (synchronizeKdeClipboard(text)) {
            // Give Klipper one event turn to publish the corrected Wayland offer.
            QTimer::singleShot(75, this, startPaste);
        } else {
            startPaste();
        }
    });
}

} // namespace kdict
