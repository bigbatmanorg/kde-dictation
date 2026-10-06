#include "app/DictationState.h"

namespace kdict {

bool DictationStateMachine::beginRecording()
{
    if (state_ != DictationState::Idle) return false;
    state_ = DictationState::Recording;
    return true;
}

bool DictationStateMachine::beginFinalizing()
{
    if (state_ != DictationState::Recording) return false;
    state_ = DictationState::Finalizing;
    return true;
}

bool DictationStateMachine::beginInserting()
{
    if (state_ != DictationState::Finalizing) return false;
    state_ = DictationState::Inserting;
    return true;
}

void DictationStateMachine::complete()
{
    state_ = DictationState::Idle;
}

void DictationStateMachine::fail()
{
    state_ = DictationState::Error;
}

void DictationStateMachine::recover()
{
    if (state_ == DictationState::Error) {
        state_ = DictationState::Idle;
    }
}

} // namespace kdict
