#pragma once

#include <QString>

namespace kdict {

enum class SessionType { Unknown, Wayland, X11 };

class Session {
public:
    // Pure classification used by detect() and by tests.
    static SessionType classify(const QString &xdgSessionType, bool hasWaylandDisplay, bool hasX11Display);

    // Inspects XDG_SESSION_TYPE, WAYLAND_DISPLAY and DISPLAY.
    static SessionType detect();
};

QString toString(SessionType type);

} // namespace kdict
