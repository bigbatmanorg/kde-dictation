#pragma once

#include "config/Config.h"
#include "output/PasteBackend.h"
#include <QObject>
#include <QStringList>
#include <QTimer>
#include <functional>
#include <memory>

namespace kdict {

class TextOutput : public QObject {
    Q_OBJECT
public:
    explicit TextOutput(QObject *parent = nullptr);

    void paste(const QString &text, const OutputConfig &config);

    // Realtime mode: deltas are coalesced and pasted serially so a later
    // clipboard update can never overtake an earlier Ctrl+V operation.
    void beginStream(const OutputConfig &config, int flushMs);
    void appendStream(const QString &text);
    void finishStream(const QString &suffix = {});
    void finishStreamToClipboard(const QString &authoritativeText);
    void abortStream();
    bool streamActive() const { return streamActive_; }

    // Resolve and prepare the key-injection backend ahead of the first paste
    // (starts a private ydotoold if necessary). Safe to call repeatedly.
    void warmUp(const OutputConfig &config);

    // Inspect the backend selected by `config` without starting anything.
    static BackendStatus backendStatus(const OutputConfig &config);

signals:
    void finished();
    void warning(const QString &message);

private:
    using PasteCallback = std::function<void(bool)>;

    static bool synchronizeKdeClipboard(const QString &text);

    PasteBackend *backendFor(const OutputConfig &config);

    void startPaste(const QString &text, const OutputConfig &config, int delayMs, PasteCallback callback);
    void scheduleStreamFlush();
    void flushStream();
    void finalizeStream();

    std::unique_ptr<PasteBackend> backend_;
    QString backendKey_;

    QTimer streamFlushTimer_;
    OutputConfig streamConfig_;
    QString streamPendingText_;
    QString streamAllText_;
    QString streamFinalClipboardText_;
    int streamFlushMs_ = 75;
    bool streamActive_ = false;
    bool streamFinishing_ = false;
    bool streamPasteInFlight_ = false;
    bool streamAutomaticPasteAvailable_ = true;
    quint64 streamGeneration_ = 0;
};

} // namespace kdict
