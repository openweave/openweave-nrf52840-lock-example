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

#include "BoltLockManager.h"
#include "WDMFeature.h"

#include "app_config.h"
#include "app_timer.h"
#include "nrf_log.h"

#include <schema/include/BoltLockTrait.h>

#include "AppTask.h"
#include "FreeRTOS.h"

BoltLockManager BoltLockManager::sLock;

APP_TIMER_DEF(sLockActuationTimer);
APP_TIMER_DEF(sAutoRelockTimer);

int BoltLockManager::Init()
{
    ret_code_t ret;

    ret = app_timer_create(&sLockActuationTimer, APP_TIMER_MODE_SINGLE_SHOT, TimerEventHandler);
    if (ret != NRF_SUCCESS)
    {
        NRF_LOG_INFO("app_timer_create() failed");
        APP_ERROR_HANDLER(ret);
    }

    ret = app_timer_create(&sAutoRelockTimer, APP_TIMER_MODE_SINGLE_SHOT, TimerEventHandler);
    if (ret != NRF_SUCCESS)
    {
        NRF_LOG_INFO("app_timer_create() failed");
        APP_ERROR_HANDLER(ret);
    }

    mState = kState_LockingCompleted;
    mAutoLockTimerArmed = false;

#if WEAVE_DEVICE_CONFIG_DISABLE_ACCOUNT_PAIRING
    mAutoRelockEnabled = AUTO_RELOCK_ENABLED_DEFAULT;
    mAutoLockDuration = AUTO_LOCK_DURATION_SECS_DEFAULT;
    mDoorCheckEnabled = DOOR_CHECK_ENABLE_DEFAULT;
#else
    mAutoRelockEnabled = false;
    mAutoLockDuration = 0;
    mDoorCheckEnabled = false;
#endif
    ButtonMgr().AddButtonEventHandler(LOCK_BUTTON, LockActionEventHandler);

    return ret;
}

bool BoltLockManager::IsActionInProgress()
{
    return (mState == kState_LockingInitiated || mState == kState_UnlockingInitiated) ? true : false;
}

bool BoltLockManager::IsUnlocked()
{
    return (mState == kState_UnlockingCompleted) ? true : false;
}

void BoltLockManager::EnableAutoRelock(bool aEnable)
{
    mAutoRelockEnabled = aEnable;
}

void BoltLockManager::EnableDoorCheck(bool aEnable)
{
    mDoorCheckEnabled = aEnable;
}

void BoltLockManager::SetAutoLockDuration(uint32_t aDurationInSecs)
{
    mAutoLockDuration = aDurationInSecs;
}

bool BoltLockManager::InitiateAction(int32_t aActor, Action_t aAction)
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
        if (mAutoLockTimerArmed && new_state == kState_LockingInitiated)
        {
            // If auto lock timer has been armed and someone initiates locking,
            // cancel the timer and continue as normal.
            CancelTimer(sAutoRelockTimer);

            mAutoLockTimerArmed = false;
        }

        StartTimer(sLockActuationTimer, ACTUATOR_MOVEMENT_PERIOS_MS);

        // Since the timer started successfully, update the state and trigger callback
        mState = new_state;

        AppEvent event;
        event.Type = AppEvent::kEventType_LockAction_Initiated;
        event.LockAction_Initiated.Action = aAction;
        event.LockAction_Initiated.Actor  = aActor;
        DispatchEvent(&event);
    }

    return action_initiated;
}

void BoltLockManager::StartTimer(app_timer_id_t aTimer, uint32_t aTimeoutMs)
{
    ret_code_t ret;
    ret = app_timer_start(aTimer, pdMS_TO_TICKS(aTimeoutMs), this);
    if (ret != NRF_SUCCESS)
    {
        NRF_LOG_INFO("app_timer_start() failed");
        APP_ERROR_HANDLER(ret);
    }
}

void BoltLockManager::CancelTimer(app_timer_id_t aTimer)
{
    ret_code_t ret;
    ret = app_timer_stop(aTimer);
    if (ret != NRF_SUCCESS)
    {
        NRF_LOG_INFO("app_timer_stop() failed");
        APP_ERROR_HANDLER(ret);
    }
}

void BoltLockManager::TimerEventHandler(void * p_context)
{
    BoltLockManager * lock = static_cast<BoltLockManager *>(p_context);

    // The timer event handler will be called in the context of the timer task
    // once sLockTimer expires. Post an event to apptask queue with the actual handler
    // so that the event can be handled in the context of the apptask.
    AppEvent event;
    event.Type               = AppEvent::kEventType_Timer;
    event.TimerEvent.Context = p_context;
    if (lock->mAutoLockTimerArmed)
    {
        event.Handler = AutoReLockTimerEventHandler;
    }
    else
    {
        event.Handler = ActuatorMovementTimerEventHandler;
    }
    GetAppTask().PostEvent(&event);
}

void BoltLockManager::AutoReLockTimerEventHandler(const AppEvent * aEvent)
{
    BoltLockManager * lock = static_cast<BoltLockManager *>(aEvent->TimerEvent.Context);
    int32_t actor = Schema::Weave::Trait::Security::BoltLockTrait::BOLT_LOCK_ACTOR_METHOD_LOCAL_IMPLICIT;

    // Make sure auto lock timer is still armed.
    if (!lock->mAutoLockTimerArmed)
    {
        return;
    }

    lock->mAutoLockTimerArmed = false;

    NRF_LOG_INFO("Auto Re-Lock has been triggered!");

    lock->InitiateAction(actor, LOCK_ACTION);
}

void BoltLockManager::ActuatorMovementTimerEventHandler(const AppEvent * aEvent)
{
    Action_t actionCompleted = INVALID_ACTION;

    BoltLockManager * lock = static_cast<BoltLockManager *>(aEvent->TimerEvent.Context);

    if (lock->mState == kState_LockingInitiated)
    {
        lock->mState    = kState_LockingCompleted;
        actionCompleted = LOCK_ACTION;
    }
    else if (lock->mState == kState_UnlockingInitiated)
    {
        lock->mState    = kState_UnlockingCompleted;
        actionCompleted = UNLOCK_ACTION;
    }

    if (actionCompleted != INVALID_ACTION)
    {
        AppEvent event;
        event.Type = AppEvent::kEventType_LockAction_Completed;
        event.LockAction_Completed.Action = actionCompleted;
        lock->DispatchEvent(&event);

        // If we are about to finish unlocking, evaluate the auto lock
        // state.
        if (actionCompleted == UNLOCK_ACTION)
            lock->EvaluateAutoRelockState(NULL);
    }
}

void BoltLockManager::PostLockActionRequest(int32_t aActor, Action_t aAction)
{
    AppEvent event;

    event.Type                          = AppEvent::kEventType_LockAction_Requested;
    event.LockAction_Requested.Actor    = aActor;
    event.LockAction_Requested.Action   = aAction;
    event.Handler                       = LockActionEventHandler;

    GetAppTask().PostEvent(&event);
}

void BoltLockManager::LockActionEventHandler(const AppEvent * aEvent)
{
    bool initiated = false;
    BoltLockManager::Action_t action;
    int32_t actor;
    ret_code_t ret = NRF_SUCCESS;

    if (aEvent->Type == AppEvent::kEventType_LockAction_Requested)
    {
        action = static_cast<BoltLockManager::Action_t>(aEvent->LockAction_Requested.Action);
        actor  = aEvent->LockAction_Requested.Actor;
    }
    else if (aEvent->Type == AppEvent::kEventType_Button && aEvent->ButtonEvent.Action == ButtonManager::kButtonAction_Release)
    {
        if (BoltLockMgr().IsUnlocked())
        {
            action = BoltLockManager::LOCK_ACTION;
        }
        else
        {
            action = BoltLockManager::UNLOCK_ACTION;
        }

        actor = Schema::Weave::Trait::Security::BoltLockTrait::BOLT_LOCK_ACTOR_METHOD_PHYSICAL;
    }
    else
    {
        ret = NRF_ERROR_NULL;
    }

    if (ret == NRF_SUCCESS)
    {
        initiated = BoltLockMgr().InitiateAction(actor, action);

        if (!initiated)
        {
            NRF_LOG_INFO("Action is already in progress or active.");
        }
    }
}

void BoltLockManager::EvaluateAutoRelockState(const AppEvent * aEvent)
{
    BoltLockManager & lock = static_cast<BoltLockManager &>(BoltLockMgr());

    // Check to see if auto relock is enabled and that the door is unlocked
    if (lock.mAutoRelockEnabled && lock.mState == kState_UnlockingCompleted)
    {
        // Check to see that the subscription to device is valid
        if (lock.mDoorCheckEnabled && WdmFeature().IsDeviceSubscriptionEstablished())
        {
            // Check to see if door is closed and that a timer is already not armed
            if (!WdmFeature().GetSecurityOpenCloseTraitDataSink().IsOpen())
            {
                // Check to see if the timer is already not armed.
                if (!lock.mAutoLockTimerArmed)
                {
                    // If device subscription is established and door is closed
                    // Start the timer for auto relock
                    lock.StartTimer(sAutoRelockTimer, lock.mAutoLockDuration * 1000);

                    lock.mAutoLockTimerArmed = true;

                    NRF_LOG_INFO("Associated O/C Sensor State is CLOSED. Auto Re-lock will be triggered " \
                                 "in %u seconds", lock.mAutoLockDuration);
                }
            }
            else
            {
                // If the state is open then we cancel the autolock timer.
                lock.CancelTimer(sAutoRelockTimer);
                lock.mAutoLockTimerArmed = false;

                NRF_LOG_INFO("Associated O/C Sensor State is OPEN. Auto Re-lock suspended.");
            }
        }
        else
        {
            // If subscription to device has not been established or if door check is disabled,
            // start the auto relock timer only if it hadn't already been started.
            if (!lock.mAutoLockTimerArmed)
            {
                // If device subscription is not valid or not established
                lock.StartTimer(sAutoRelockTimer, lock.mAutoLockDuration * 1000);

                lock.mAutoLockTimerArmed = true;

                NRF_LOG_INFO("Auto Re-lock will be triggered in %u seconds", lock.mAutoLockDuration);
            }
        }
    }
}

void BoltLockManager::EvaluateChange(void)
{
    AppEvent event;
    event.Type = AppEvent::kEventType_AutoReLock_Evaluate;
    event.Handler = EvaluateAutoRelockState;

    GetAppTask().PostEvent(&event);
}