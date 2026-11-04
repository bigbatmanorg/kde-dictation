#include "output/PasteBackend.h"

#include "output/XdotoolBackend.h"
#include "output/YdotoolBackend.h"
#include "util/Session.h"

namespace kdict {

QString PasteBackendFactory::resolveName(const QString &configured)
{
    const QString wanted = configured.trimmed().toLower();
    if (wanted == QStringLiteral("ydotool") || wanted == QStringLiteral("xdotool")) return wanted;
    // auto: X11 sessions prefer xdotool (no uinput permissions needed) when
    // installed; everything else, notably Wayland, uses ydotool.
    if (Session::detect() == SessionType::X11 && !XdotoolBackend::executable().isEmpty()) {
        return QStringLiteral("xdotool");
    }
    return QStringLiteral("ydotool");
}

std::unique_ptr<PasteBackend> PasteBackendFactory::create(const OutputConfig &config, QObject *parent)
{
    if (resolveName(config.backend) == QStringLiteral("xdotool")) {
        return std::make_unique<XdotoolBackend>(parent);
    }
    return std::make_unique<YdotoolBackend>(config, parent);
}

} // namespace kdict
