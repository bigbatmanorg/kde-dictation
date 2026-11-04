#pragma once

#include <QString>
#include <QStringList>

namespace kdict {

// ydotool's socket location differs between distributions and versions:
//   * upstream ydotoold (user session):   $XDG_RUNTIME_DIR/.ydotool_socket
//   * Fedora/Arch system service (root):  /tmp/.ydotool_socket
//   * custom setups:                      $YDOTOOL_SOCKET
// This helper enumerates the candidates and picks the first one a client can
// actually talk to, instead of assuming a single hard-coded path.
class YdotoolSocket {
public:
    // Socket owned by this application when it has to start ydotoold itself.
    static QString managedPath(const QString &runtimeDir);

    // Ordered candidate list (configured path first, application-managed path
    // last among the per-user locations, world-visible /tmp last).
    static QStringList candidates(const QString &configured,
                                  const QString &envOverride,
                                  const QString &runtimeDir);

    // True when a datagram/stream client can connect to the unix socket, i.e.
    // a daemon is really listening. Stale socket files return false.
    static bool probe(const QString &path);

    // First candidate for which `usable` returns true; empty when none.
    template <typename Predicate>
    static QString firstUsable(const QStringList &candidates, Predicate usable)
    {
        for (const QString &path : candidates) {
            if (usable(path)) return path;
        }
        return {};
    }
};

} // namespace kdict
