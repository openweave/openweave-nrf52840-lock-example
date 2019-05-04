#include "LockWidget.h"
#include "app_timer.h"
#include "nrf_log.h"

#include "FreeRTOS.h"


#define ACTION_ACTIVATION_TIMEOUT_MS    2000

APP_TIMER_DEF(sLockTimer);

int LockWidget::Init()
{
    ret_code_t ret;

    ret = app_timer_create(&sLockTimer, APP_TIMER_MODE_SINGLE_SHOT, LockTimerEventHandler);
    if (ret != NRF_SUCCESS)
    {
        NRF_LOG_INFO("app_timer_create() failed");
    }

    mState = kState_LockingCompleted;

    return ret;
}

void LockWidget::SetCallbacks(Callback_fn_initiated aActionInitiated_CB,
                              Callback_fn_completed aActionCompleted_CB)
{
    mActionInitiated_CB = aActionInitiated_CB;
    mActionCompleted_CB = aActionCompleted_CB;
}

LockWidget::State_t LockWidget::GetState()
{
    return mState;
}

bool LockWidget::IsActionInProgress()
{
    return (mState == kState_LockingInitiated || mState == kState_UnlockingInitiated) ?
        true : false;
}

bool LockWidget::IsUnlocked()
{
    return (mState == kState_UnlockingCompleted) ? true : false;
}

bool LockWidget::InitiateAction(int32_t aActor, Action_t aAction)
{
    bool action_initiated = false;
    State_t new_state;

    // Initiate Lock/Unlock Action only when the previous one is complete.
    if (mState == kState_LockingCompleted && aAction == UNLOCK_ACTION)
    {
        action_initiated = true;

        new_state = kState_UnlockingInitiated;
    }
    else if (mState == kState_UnlockingCompleted && aAction == LOCK_ACTION)
    {
        action_initiated = true;

        new_state = kState_LockingInitiated;
    }

    if (action_initiated)
    {
        ret_code_t ret;
        ret = app_timer_start(sLockTimer, pdMS_TO_TICKS(ACTION_ACTIVATION_TIMEOUT_MS), this);
        if (ret != NRF_SUCCESS)
        {
            NRF_LOG_INFO("app_timer_start() failed");
            APP_ERROR_HANDLER(ret);

            action_initiated = false;
        }
        else
        {
            NRF_LOG_INFO("Activation In Progress");
            // Since the timer started successfully, update the state and trigger callback
            mState = new_state;

            if (mActionInitiated_CB)
            {
                mActionInitiated_CB(aAction, aActor);
            }
        }
    }

    return action_initiated;
}

void LockWidget::LockTimerEventHandler(void * p_context)
{
    Action_t actionCompleted = INVALID_ACTION;
    LockWidget * lock = static_cast<LockWidget *>(p_context);

    if (lock->mState == kState_LockingInitiated)
    {
        lock->mState = kState_LockingCompleted;
        actionCompleted = LOCK_ACTION;
    }
    else if (lock->mState == kState_UnlockingInitiated)
    {
        lock->mState = kState_UnlockingCompleted;
        actionCompleted = UNLOCK_ACTION;
    }

    if (actionCompleted != INVALID_ACTION)
    {
        if (lock->mActionCompleted_CB)
        {
            lock->mActionCompleted_CB(actionCompleted);
        }
    }
}
