#include "output/YdotoolSocket.h"

#include <QByteArray>
#include <cerrno>
#include <cstring>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

namespace kdict {

QString YdotoolSocket::managedPath(const QString &runtimeDir)
{
    if (runtimeDir.isEmpty()) return {};
    return runtimeDir + QStringLiteral("/kde-dictation-ydotool.sock");
}

QStringList YdotoolSocket::candidates(const QString &configured,
                                      const QString &envOverride,
                                      const QString &runtimeDir)
{
    QStringList result;
    const auto add = [&result](const QString &path) {
        const QString trimmed = path.trimmed();
        if (!trimmed.isEmpty() && !result.contains(trimmed)) result << trimmed;
    };
    add(configured);
    add(envOverride);
    if (!runtimeDir.isEmpty()) {
        add(runtimeDir + QStringLiteral("/.ydotool_socket"));
        add(managedPath(runtimeDir));
    }
    add(QStringLiteral("/tmp/.ydotool_socket"));
    return result;
}

bool YdotoolSocket::probe(const QString &path)
{
    const QByteArray raw = path.toUtf8();
    sockaddr_un addr{};
    if (raw.isEmpty() || static_cast<size_t>(raw.size()) >= sizeof(addr.sun_path)) return false;
    addr.sun_family = AF_UNIX;
    std::memcpy(addr.sun_path, raw.constData(), static_cast<size_t>(raw.size()));

    for (const int type : {SOCK_DGRAM, SOCK_STREAM}) {
        const int fd = ::socket(AF_UNIX, type | SOCK_CLOEXEC, 0);
        if (fd < 0) return false;
        const int rc = ::connect(fd, reinterpret_cast<const sockaddr *>(&addr), sizeof(addr));
        const int err = errno;
        ::close(fd);
        if (rc == 0) return true;
        // EPROTOTYPE: the socket exists but is of the other type; try it.
        if (err != EPROTOTYPE) return false;
    }
    return false;
}

} // namespace kdict
