#pragma once

namespace kdict {

enum class DictationState {
    Idle,
    Recording,
    Finalizing,
    Inserting,
    Error,
};

class DictationStateMachine {
public:
    DictationState state() const { return state_; }

    bool beginRecording();
    bool beginFinalizing();
    bool beginInserting();
    void complete();
    void fail();
    void recover();

private:
    DictationState state_ = DictationState::Idle;
};

} // namespace kdict
