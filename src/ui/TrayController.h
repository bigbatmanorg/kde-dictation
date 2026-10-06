#pragma once

#include <QObject>
#include <QElapsedTimer>
#include <QTimer>

class KStatusNotifierItem;
class QMenu;
class QAction;

namespace kdict {

class TrayController : public QObject {
    Q_OBJECT
public:
    explicit TrayController(QObject *parent = nullptr);

    void showIdle();
    void showRecording();
    void showFinalizing(const QString &detail = {});
    void showInserting();
    void showError(const QString &message);
    void showInfo(const QString &message);

signals:
    void toggleRequested();
    void reloadRequested();
    void quitRequested();

private:
    void updateRecordingTooltip();

    KStatusNotifierItem *item_ = nullptr;
    QMenu *menu_ = nullptr;
    QAction *toggleAction_ = nullptr;
    QElapsedTimer recordingElapsed_;
    QTimer recordingTimer_;
};

} // namespace kdict
