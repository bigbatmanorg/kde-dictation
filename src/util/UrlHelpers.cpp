#include "util/UrlHelpers.h"

namespace kdict {
namespace {
QUrl withPath(const QUrl &base, const QString &path)
{
    QUrl url(base);
    QString normalized = path.trimmed();
    if (!normalized.startsWith('/')) normalized.prepend('/');
    url.setPath(normalized);
    url.setQuery({});
    url.setFragment({});
    return url;
}
}

QUrl makeHttpEndpoint(const QUrl &base, const QString &path)
{
    QUrl url = withPath(base, path);
    if (url.scheme() == QStringLiteral("ws")) url.setScheme(QStringLiteral("http"));
    if (url.scheme() == QStringLiteral("wss")) url.setScheme(QStringLiteral("https"));
    return url;
}

QUrl makeWebSocketEndpoint(const QUrl &base, const QString &path)
{
    QUrl url = withPath(base, path);
    if (url.scheme() == QStringLiteral("http")) url.setScheme(QStringLiteral("ws"));
    if (url.scheme() == QStringLiteral("https")) url.setScheme(QStringLiteral("wss"));
    return url;
}

} // namespace kdict
