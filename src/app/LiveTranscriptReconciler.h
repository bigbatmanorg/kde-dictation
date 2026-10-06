#pragma once

#include <QString>

namespace kdict {

enum class LiveReconcileKind {
    Exact,
    EquivalentWhitespace,
    AppendSuffix,
    StreamedLonger,
    Mismatch,
};

struct LiveReconcileResult {
    LiveReconcileKind kind = LiveReconcileKind::Exact;
    QString suffix;
};

class LiveTranscriptReconciler {
public:
    static LiveReconcileResult reconcile(const QString &streamed, const QString &finalText);
};

} // namespace kdict
