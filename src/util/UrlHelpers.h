#pragma once

#include <QUrl>
#include <QString>

namespace kdict {

QUrl makeHttpEndpoint(const QUrl &base, const QString &path);
QUrl makeWebSocketEndpoint(const QUrl &base, const QString &path);

} // namespace kdict
