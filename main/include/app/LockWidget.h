#ifndef LOCK_WIDGET_H
#define LOCK_WIDGET_H

#include <stdint.h>
#include <stdbool.h>

class LockWidget
{
public:

    enum Action_t {
        LOCK_ACTION = 0,
        UNLOCK_ACTION,

        INVALID_ACTION
    }Action;

    enum State_t {
        kState_LockingInitiated = 0,
        kState_LockingCompleted,
        kState_UnlockingInitiated,
        kState_UnlockingCompleted,
    } State;

    int Init();
    State_t GetState();
    bool IsActionInProgress();
    bool IsUnlocked();
    bool InitiateAction(int32_t aActor, Action_t aAction);

    typedef void (*Callback_fn_initiated)(Action_t, int32_t aActor);
    typedef void (*Callback_fn_completed)(Action_t);
    void SetCallbacks(Callback_fn_initiated aActionInitiated_CB,
                      Callback_fn_completed aActionCompleted_CB);

private:

    State_t mState;

    Callback_fn_initiated mActionInitiated_CB;
    Callback_fn_completed mActionCompleted_CB;

    static void LockTimerEventHandler(void * p_context);

};

#endif // LOCK_WIDGET_H
