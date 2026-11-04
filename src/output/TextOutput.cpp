#include "output/TextOutput.h"

#include <QClipboard>
#include <QDBusInterface>
#include <QDBusReply>
#include <QGuiApplication>
#include <QDebug>
#include <QTimer>
#include <memory>

namespace kdict {

TextOutput::TextOutput(QObject *parent) : QObject(parent)
{
    streamFlushTimer_.setSingleShot(true);
    connect(&streamFlushTimer_, &QTimer::timeout, this, &TextOutput::flushStream);
}

namespace {

QString backendKeyOf(const OutputConfig &config)
{
    return config.backend + QLatin1Char('|') + config.ydotoolSocket + QLatin1Char('|')
        + (config.ydotoolAutostart ? QLatin1Char('1') : QLatin1Char('0'));
}

} // namespace

PasteBackend *TextOutput::backendFor(const OutputConfig &config)
{
    const QString key = backendKeyOf(config);
    if (!backend_ || key != backendKey_) {
        backend_ = PasteBackendFactory::create(config, this);
        backendKey_ = key;
    }
    return backend_.get();
}

void TextOutput::warmUp(const OutputConfig &config)
{
    PasteBackend *backend = backendFor(config);
    QString error;
    if (!backend->prepare(&error)) {
        qWarning().noquote() << "Paste backend" << backend->name() << "not ready:" << error;
    } else {
        qInfo().noquote() << "Paste backend:" << backend->name();
    }
}

BackendStatus TextOutput::backendStatus(const OutputConfig &config)
{
    return PasteBackendFactory::create(config)->status();
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

    auto callbackState = std::make_shared<PasteCallback>(std::move(callback));
    const auto complete = [callbackState](bool ok) {
        if (!*callbackState) return;
        auto cb = std::move(*callbackState);
        *callbackState = {};
        cb(ok);
    };

    PasteBackend *backend = backendFor(config);
    const auto startProcess = [this, backend, config, complete]() mutable {
        backend->sendPaste(config.pasteShortcut, [this, complete](bool ok, const QString &error) mutable {
            if (!ok) {
                emit warning(QStringLiteral("Automatic paste failed; transcript remains in the clipboard%1")
                                 .arg(error.isEmpty() ? QString() : QStringLiteral(": %1").arg(error)));
            }
            complete(ok);
        });
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
