#include "output/TextOutput.h"

#include <QClipboard>
#include <QDBusInterface>
#include <QDBusReply>
#include <QFileInfo>
#include <QGuiApplication>
#include <QProcess>
#include <QStandardPaths>
#include <QTimer>
#include <memory>

namespace kdict {

TextOutput::TextOutput(QObject *parent) : QObject(parent)
{
    streamFlushTimer_.setSingleShot(true);
    connect(&streamFlushTimer_, &QTimer::timeout, this, &TextOutput::flushStream);
}

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
    QDBusInterface klipper(QStringLiteral("org.kde.klipper"),
                            QStringLiteral("/klipper"),
                            QStringLiteral("org.kde.klipper.klipper"));
    if (!klipper.isValid()) return false;

    const QDBusReply<QString> current = klipper.call(QStringLiteral("getClipboardContents"));
    if (!current.isValid() || current.value() == text) return false;

    const QDBusMessage reply = klipper.call(QStringLiteral("setClipboardContents"), text);
    return reply.type() != QDBusMessage::ErrorMessage;
}

void TextOutput::startPaste(const QString &text, const OutputConfig &config, int delayMs, PasteCallback callback)
{
    QGuiApplication::clipboard()->setText(text, QClipboard::Clipboard);

    const QString executable = ydotoolExecutable();
    if (executable.isEmpty()) {
        emit warning(QStringLiteral("ydotool not found; transcript remains in the clipboard"));
        QTimer::singleShot(0, this, [callback = std::move(callback)]() mutable { callback(false); });
        return;
    }

    const QString socket = ydotoolSocketPath();
    if (socket.isEmpty() || !QFileInfo::exists(socket)) {
        emit warning(QStringLiteral("ydotoold socket not found; transcript remains in the clipboard"));
        QTimer::singleShot(0, this, [callback = std::move(callback)]() mutable { callback(false); });
        return;
    }

    auto callbackState = std::make_shared<PasteCallback>(std::move(callback));
    const auto complete = [callbackState](bool ok) {
        if (!*callbackState) return;
        auto cb = std::move(*callbackState);
        *callbackState = {};
        cb(ok);
    };

    const auto startProcess = [this, executable, config, complete]() mutable {
        auto *process = new QProcess(this);
        connect(process, &QProcess::finished, this,
                [this, process, complete](int exitCode, QProcess::ExitStatus status) mutable {
            const QString stderrText = QString::fromUtf8(process->readAllStandardError()).trimmed();
            process->deleteLater();
            const bool ok = status == QProcess::NormalExit && exitCode == 0;
            if (!ok) {
                emit warning(QStringLiteral("ydotool paste failed; transcript remains in clipboard%1")
                                 .arg(stderrText.isEmpty() ? QString() : QStringLiteral(": %1").arg(stderrText)));
            }
            complete(ok);
        });
        process->start(executable, ydotoolArgs(config.pasteShortcut));
        if (!process->waitForStarted(500)) {
            const QString reason = process->errorString();
            QObject::disconnect(process, nullptr, this, nullptr);
            process->deleteLater();
            emit warning(QStringLiteral("Could not start ydotool; transcript remains in clipboard: %1").arg(reason));
            complete(false);
        }
    };

    QTimer::singleShot(qMax(0, delayMs), this, [this, text, startProcess = std::move(startProcess)]() mutable {
        if (synchronizeKdeClipboard(text)) {
            // Give Klipper one event turn to publish the corrected Wayland offer.
            QTimer::singleShot(35, this, std::move(startProcess));
        } else {
            startProcess();
        }
    });
}

void TextOutput::paste(const QString &text, const OutputConfig &config)
{
    startPaste(text, config, config.pasteDelayMs, [this](bool) { emit finished(); });
}

void TextOutput::beginStream(const OutputConfig &config, int flushMs)
{
    abortStream();
    ++streamGeneration_;
    streamConfig_ = config;
    streamFlushMs_ = qMax(0, flushMs);
    streamActive_ = true;
    streamAutomaticPasteAvailable_ = true;
}

void TextOutput::appendStream(const QString &text)
{
    if (!streamActive_ || streamFinishing_ || text.isEmpty()) return;
    streamAllText_ += text;
    if (!streamAutomaticPasteAvailable_) return;
    streamPendingText_ += text;
    scheduleStreamFlush();
}

void TextOutput::finishStream(const QString &suffix)
{
    if (!streamActive_) {
        if (!suffix.isEmpty()) paste(suffix, streamConfig_);
        else emit finished();
        return;
    }

    if (!suffix.isEmpty()) {
        streamAllText_ += suffix;
        if (streamAutomaticPasteAvailable_) streamPendingText_ += suffix;
    }
    streamFinishing_ = true;
    streamFlushTimer_.stop();

    if (!streamAutomaticPasteAvailable_) {
        finalizeStream();
    } else if (!streamPasteInFlight_ && !streamPendingText_.isEmpty()) {
        flushStream();
    } else if (!streamPasteInFlight_) {
        finalizeStream();
    }
}

void TextOutput::finishStreamToClipboard(const QString &authoritativeText)
{
    if (!streamActive_) {
        QGuiApplication::clipboard()->setText(authoritativeText, QClipboard::Clipboard);
        synchronizeKdeClipboard(authoritativeText);
        emit finished();
        return;
    }

    // A final transcript mismatch means we no longer know that queued deltas are
    // safe to append. Stop queueing new text and leave the authoritative result
    // in the clipboard instead of destructively editing the focused control.
    streamFinalClipboardText_ = authoritativeText;
    streamPendingText_.clear();
    streamFinishing_ = true;
    streamFlushTimer_.stop();
    if (!streamPasteInFlight_) finalizeStream();
}

void TextOutput::abortStream()
{
    streamFlushTimer_.stop();
    ++streamGeneration_;
    if (streamActive_ && !streamAllText_.isEmpty()) {
        QGuiApplication::clipboard()->setText(streamAllText_, QClipboard::Clipboard);
        synchronizeKdeClipboard(streamAllText_);
    }
    streamPendingText_.clear();
    streamAllText_.clear();
    streamFinalClipboardText_.clear();
    streamActive_ = false;
    streamFinishing_ = false;
    streamPasteInFlight_ = false;
    streamAutomaticPasteAvailable_ = true;
}

void TextOutput::scheduleStreamFlush()
{
    if (!streamActive_ || streamFinishing_ || streamPasteInFlight_ || streamPendingText_.isEmpty()) return;
    if (!streamFlushTimer_.isActive()) streamFlushTimer_.start(streamFlushMs_);
}

void TextOutput::flushStream()
{
    if (!streamActive_ || streamPasteInFlight_ || !streamAutomaticPasteAvailable_) return;
    if (streamPendingText_.isEmpty()) {
        if (streamFinishing_) finalizeStream();
        return;
    }

    const QString chunk = streamPendingText_;
    streamPendingText_.clear();
    streamPasteInFlight_ = true;
    const quint64 generation = streamGeneration_;

    startPaste(chunk, streamConfig_, streamConfig_.livePasteDelayMs,
               [this, generation](bool success) {
        if (generation != streamGeneration_ || !streamActive_) return;
        streamPasteInFlight_ = false;
        if (!success) {
            streamAutomaticPasteAvailable_ = false;
            streamPendingText_.clear();
        }

        if (streamFinishing_) {
            if (streamAutomaticPasteAvailable_ && !streamPendingText_.isEmpty()) flushStream();
            else finalizeStream();
        } else if (streamAutomaticPasteAvailable_ && !streamPendingText_.isEmpty()) {
            scheduleStreamFlush();
        }
    });
}

void TextOutput::finalizeStream()
{
    if (!streamActive_ || streamPasteInFlight_) return;

    const QString clipboardText = !streamFinalClipboardText_.isEmpty()
        ? streamFinalClipboardText_
        : streamAllText_;
    if (!clipboardText.isEmpty()) {
        QGuiApplication::clipboard()->setText(clipboardText, QClipboard::Clipboard);
        synchronizeKdeClipboard(clipboardText);
    }

    streamFlushTimer_.stop();
    streamPendingText_.clear();
    streamActive_ = false;
    streamFinishing_ = false;
    streamAutomaticPasteAvailable_ = true;
    emit finished();
}

} // namespace kdict
