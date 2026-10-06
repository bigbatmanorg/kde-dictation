#pragma once

#include <QString>

namespace kdict {

enum class RealtimeEventKind {
    Unknown,
    SessionCreated,
    SessionUpdated,
    Delta,
    Completed,
    Committed,
    Error,
};

struct RealtimeEvent {
    RealtimeEventKind kind = RealtimeEventKind::Unknown;
    QString type;
    QString text;
    QString errorMessage;
};

class RealtimeEventParser {
public:
    static RealtimeEvent parse(const QString &message);
};

} // namespace kdict
