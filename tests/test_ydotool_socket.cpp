#include <QtTest>
#include <QTemporaryDir>
#include "output/YdotoolSocket.h"

#include <cstring>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

using namespace kdict;

namespace {
int bindSocket(const QString &path, int type)
{
    const int fd = ::socket(AF_UNIX, type, 0);
    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    const QByteArray raw = path.toUtf8();
    std::memcpy(addr.sun_path, raw.constData(), static_cast<size_t>(raw.size()));
    if (fd < 0 || ::bind(fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) != 0) return -1;
    if (type == SOCK_STREAM) ::listen(fd, 1);
    return fd;
}
}

class YdotoolSocketTest : public QObject {
    Q_OBJECT
private slots:
    void candidateOrderAndDedup()
    {
        const QStringList c = YdotoolSocket::candidates(QStringLiteral("/custom.sock"),
                                                        QStringLiteral("/env.sock"),
                                                        QStringLiteral("/run/user/1000"));
        const QStringList expected{"/custom.sock", "/env.sock", "/run/user/1000/.ydotool_socket",
                                   "/run/user/1000/kde-dictation-ydotool.sock", "/tmp/.ydotool_socket"};
        QCOMPARE(c, expected);

        const QStringList dup = YdotoolSocket::candidates(QString(), QStringLiteral("/tmp/.ydotool_socket"), QString());
        QCOMPARE(dup, QStringList{"/tmp/.ydotool_socket"});
    }
    void managedPathNeedsRuntimeDir()
    {
        QVERIFY(YdotoolSocket::managedPath(QString()).isEmpty());
        QCOMPARE(YdotoolSocket::managedPath(QStringLiteral("/run/user/1")), QStringLiteral("/run/user/1/kde-dictation-ydotool.sock"));
    }
    void probeLiveDatagramAndStale()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString path = dir.filePath(QStringLiteral("d.sock"));
        QVERIFY(!YdotoolSocket::probe(path)); // missing
        const int fd = bindSocket(path, SOCK_DGRAM);
        QVERIFY(fd >= 0);
        QVERIFY(YdotoolSocket::probe(path));
        ::close(fd);
        // The socket file is left behind, but nothing listens: stale.
        QVERIFY(QFileInfo::exists(path));
        QVERIFY(!YdotoolSocket::probe(path));
    }
    void probeLiveStream()
    {
        QTemporaryDir dir;
        const QString path = dir.filePath(QStringLiteral("s.sock"));
        const int fd = bindSocket(path, SOCK_STREAM);
        QVERIFY(fd >= 0);
        QVERIFY(YdotoolSocket::probe(path));
        ::close(fd);
    }
    void firstUsablePicksFirstMatch()
    {
        const QStringList list{"/a", "/b", "/c"};
        QCOMPARE(YdotoolSocket::firstUsable(list, [](const QString &p) { return p != QStringLiteral("/a"); }), QStringLiteral("/b"));
        QVERIFY(YdotoolSocket::firstUsable(list, [](const QString &) { return false; }).isEmpty());
    }
};

QTEST_MAIN(YdotoolSocketTest)
#include "test_ydotool_socket.moc"
