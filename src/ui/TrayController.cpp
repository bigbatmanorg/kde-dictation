#include "ui/TrayController.h"

#include <KStatusNotifierItem>
#include <QAction>
#include <QCoreApplication>
#include <QMenu>

namespace kdict {

TrayController::TrayController(QObject *parent) : QObject(parent)
{
    item_ = new KStatusNotifierItem(QStringLiteral("kde-dictation"), this);
    item_->setCategory(KStatusNotifierItem::ApplicationStatus);
    item_->setTitle(QStringLiteral("KDE Dictation"));
    item_->setStandardActionsEnabled(false);

    menu_ = new QMenu;
    toggleAction_ = menu_->addAction(QStringLiteral("Start recording"));
    auto *reload = menu_->addAction(QStringLiteral("Reload configuration"));
    menu_->addSeparator();
    auto *quit = menu_->addAction(QStringLiteral("Quit"));
    item_->setContextMenu(menu_);

    connect(toggleAction_, &QAction::triggered, this, &TrayController::toggleRequested);
    connect(reload, &QAction::triggered, this, &TrayController::reloadRequested);
    connect(quit, &QAction::triggered, this, &TrayController::quitRequested);

    recordingTimer_.setInterval(1000);
    connect(&recordingTimer_, &QTimer::timeout, this, &TrayController::updateRecordingTooltip);
    showIdle();
}

void TrayController::showIdle()
{
    recordingTimer_.stop();
    item_->setIconByName(QStringLiteral("audio-input-microphone"));
    item_->setStatus(KStatusNotifierItem::Active);
    item_->setToolTip(QStringLiteral("audio-input-microphone"), QStringLiteral("KDE Dictation"), QStringLiteral("Idle"));
    toggleAction_->setText(QStringLiteral("Start recording"));
}

void TrayController::showRecording()
{
    recordingElapsed_.restart();
    recordingTimer_.start();
    item_->setIconByName(QStringLiteral("media-record"));
    item_->setStatus(KStatusNotifierItem::NeedsAttention);
    toggleAction_->setText(QStringLiteral("Stop recording"));
    updateRecordingTooltip();
}

void TrayController::updateRecordingTooltip()
{
    const qint64 seconds = recordingElapsed_.isValid() ? recordingElapsed_.elapsed() / 1000 : 0;
    const QString time = QStringLiteral("%1:%2")
                             .arg(seconds / 60, 2, 10, QLatin1Char('0'))
                             .arg(seconds % 60, 2, 10, QLatin1Char('0'));
    item_->setToolTip(QStringLiteral("media-record"), QStringLiteral("KDE Dictation"), QStringLiteral("Recording — %1").arg(time));
}

void TrayController::showFinalizing(const QString &detail)
{
    recordingTimer_.stop();
    item_->setIconByName(QStringLiteral("view-refresh"));
    item_->setStatus(KStatusNotifierItem::Active);
    item_->setToolTip(QStringLiteral("view-refresh"), QStringLiteral("KDE Dictation"),
                      detail.isEmpty() ? QStringLiteral("Transcribing…") : detail);
    toggleAction_->setText(QStringLiteral("Busy"));
}

void TrayController::showInserting()
{
    item_->setIconByName(QStringLiteral("edit-paste"));
    item_->setStatus(KStatusNotifierItem::Active);
    item_->setToolTip(QStringLiteral("edit-paste"), QStringLiteral("KDE Dictation"), QStringLiteral("Inserting transcript…"));
}

void TrayController::showError(const QString &message)
{
    recordingTimer_.stop();
    item_->setIconByName(QStringLiteral("dialog-error"));
    item_->setStatus(KStatusNotifierItem::NeedsAttention);
    item_->setToolTip(QStringLiteral("dialog-error"), QStringLiteral("KDE Dictation"), message);
    item_->showMessage(QStringLiteral("KDE Dictation"), message, QStringLiteral("dialog-error"), 8000);
    toggleAction_->setText(QStringLiteral("Start recording"));
}

void TrayController::showInfo(const QString &message)
{
    item_->showMessage(QStringLiteral("KDE Dictation"), message, QStringLiteral("dialog-information"), 5000);
}

} // namespace kdict
