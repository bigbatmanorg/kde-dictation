#include "util/Session.h"

#include <QtGlobal>

namespace kdict {

SessionType Session::classify(const QString &xdgSessionType, bool hasWaylandDisplay, bool hasX11Display)
{
    const QString declared = xdgSessionType.trimmed().toLower();
    if (declared == QStringLiteral("wayland")) return SessionType::Wayland;
    if (declared == QStringLiteral("x11")) return SessionType::X11;
    // tty/unspecified/empty: fall back to the display variables.
    if (hasWaylandDisplay) return SessionType::Wayland;
    if (hasX11Display) return SessionType::X11;
    return SessionType::Unknown;
}

SessionType Session::detect()
{
    return classify(qEnvironmentVariable("XDG_SESSION_TYPE"),
                    !qEnvironmentVariable("WAYLAND_DISPLAY").isEmpty(),
                    !qEnvironmentVariable("DISPLAY").isEmpty());
}

QString toString(SessionType type)
{
    switch (type) {
    case SessionType::Wayland: return QStringLiteral("wayland");
    case SessionType::X11: return QStringLiteral("x11");
    case SessionType::Unknown: break;
    }
    return QStringLiteral("unknown");
}

} // namespace kdict
