/*
 *
 *    Copyright (c) 2019 Google LLC.
 *    All rights reserved.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include "LockWidget.h"

#include "app_config.h"
#include "app_timer.h"
#include "nrf_log.h"

#include "AppTask.h"
#include "FreeRTOS.h"

APP_TIMER_DEF(sLockTimer);

int LockWidget::Init()
{
    ret_code_t ret;

    ret = app_timer_create(&sLockTimer, APP_TIMER_MODE_SINGLE_SHOT, TimerEventHandler);
    if (ret != NRF_SUCCESS)
    {
        NRF_LOG_INFO("app_timer_create() failed");
        APP_ERROR_HANDLER(ret);
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
        ret = app_timer_start(sLockTimer, pdMS_TO_TICKS(ACTUATOR_MOVEMENT_PERIOS_MS), this);
        if (ret != NRF_SUCCESS)
        {
            NRF_LOG_INFO("app_timer_start() failed");
            APP_ERROR_HANDLER(ret);

            action_initiated = false;
        }
        else
        {
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

void LockWidget::TimerEventHandler(void * p_context)
{
    // The timer event handler will be called in the context of the timer task
    // once sLockTimer expires. Post an event to apptask queue with the actual handler
    // so that the event can be handled in the context of the apptask.
    AppEvent event;
    event.Type = AppEvent::kEventType_Timer;
    event.TimerEvent.Context = p_context;
    event.Handler = LockTimerEventHandler;
    GetAppTask().PostEvent(&event);
}

void LockWidget::LockTimerEventHandler(AppEvent *aEvent)
{
    Action_t actionCompleted = INVALID_ACTION;

    LockWidget * lock = static_cast<LockWidget *>(aEvent->TimerEvent.Context);

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
