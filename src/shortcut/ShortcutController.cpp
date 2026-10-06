#include "shortcut/ShortcutController.h"

#include <QAction>
#include <QKeySequence>
#include <KGlobalAccel>

namespace kdict {

ShortcutController::ShortcutController(QObject *parent) : QObject(parent)
{
    action_ = new QAction(QStringLiteral("Dictate"), this);
    action_->setObjectName(QStringLiteral("dictate"));
    action_->setProperty("componentDisplayName", QStringLiteral("KDE Dictation"));

    const QList<QKeySequence> defaultShortcut{QKeySequence(QStringLiteral("Meta+Shift+D"))};
    // The default is shown in Plasma's shortcut settings; Autoloading restores any
    // user-selected active binding instead of overwriting it at the next startup.
    KGlobalAccel::self()->setDefaultShortcut(action_, defaultShortcut);
    KGlobalAccel::self()->setShortcut(action_, defaultShortcut, KGlobalAccel::Autoloading);

    connect(KGlobalAccel::self(), &KGlobalAccel::globalShortcutActiveChanged,
            this, [this](QAction *action, bool active) {
        if (action != action_) return;
        if (mode_ == ActivationMode::Toggle) {
            if (active) emit toggleRequested();
            return;
        }
        if (active) emit startRequested();
        else emit stopRequested();
    });
}

} // namespace kdict
