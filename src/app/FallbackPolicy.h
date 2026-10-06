#pragma once

#include "app/DictationState.h"

namespace kdict {

enum class FallbackAction {
    None,
    DeferUntilStop,
    StartBatchNow,
};

class FallbackPolicy {
public:
    static FallbackAction onRealtimeFailure(DictationState state, bool enabled);
};

} // namespace kdict
