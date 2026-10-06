#include "app/LiveTranscriptReconciler.h"

namespace kdict {

LiveReconcileResult LiveTranscriptReconciler::reconcile(const QString &streamed, const QString &finalText)
{
    if (streamed == finalText) {
        return {LiveReconcileKind::Exact, {}};
    }
    if (streamed.trimmed() == finalText.trimmed()) {
        return {LiveReconcileKind::EquivalentWhitespace, {}};
    }
    if (finalText.startsWith(streamed)) {
        return {LiveReconcileKind::AppendSuffix, finalText.mid(streamed.size())};
    }
    if (streamed.startsWith(finalText)) {
        return {LiveReconcileKind::StreamedLonger, {}};
    }
    return {LiveReconcileKind::Mismatch, {}};
}

} // namespace kdict
