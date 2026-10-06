#include "app/FallbackPolicy.h"

namespace kdict {

FallbackAction FallbackPolicy::onRealtimeFailure(DictationState state, bool enabled)
{
    if (!enabled) return FallbackAction::None;
    if (state == DictationState::Recording) return FallbackAction::DeferUntilStop;
    if (state == DictationState::Finalizing) return FallbackAction::StartBatchNow;
    return FallbackAction::None;
}

} // namespace kdict
