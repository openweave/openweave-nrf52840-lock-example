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

#ifndef LOCK_MANAGER_H
#define LOCK_MANAGER_H

#include <stdint.h>
#include <stdbool.h>

// nRF SDK includes
#include "app_timer.h"

#include "AppEvent.h"

class BoltLockManager : public AppEventDispatcher
{
public:
    enum Action_t
    {
        LOCK_ACTION = 0,
        UNLOCK_ACTION,

        INVALID_ACTION
    } Action;

    enum State_t
    {
        kState_LockingInitiated = 0,
        kState_LockingCompleted,
        kState_UnlockingInitiated,
        kState_UnlockingCompleted,
    } State;

    int Init();
    bool IsUnlocked();
    void EvaluateChange(void);
    void EnableAutoRelock(bool aEnable);
    void EnableDoorCheck(bool aEnable);
    void SetAutoLockDuration(uint32_t aDurationInSecs);
    bool IsActionInProgress();
    bool InitiateAction(int32_t aActor, Action_t aAction);

    void PostLockActionRequest(int32_t aActor, Action_t aAction);

private:
    friend BoltLockManager & BoltLockMgr(void);
    State_t mState;

    bool mAutoRelockEnabled;
    bool mDoorCheckEnabled;
    bool mAutoLockTimerArmed;

    uint32_t mAutoLockDuration;

    void CancelTimer(app_timer_id_t aTimer);
    void StartTimer(app_timer_id_t aTimer, uint32_t aTimeoutMs);

    static void TimerEventHandler(void * p_context);
    static void EvaluateAutoRelockState(const AppEvent * aEvent);
    static void AutoReLockTimerEventHandler(const AppEvent * aEvent);
    static void ActuatorMovementTimerEventHandler(const AppEvent *aEvent);
    static void LockActionEventHandler(const AppEvent * aEvent);

    static BoltLockManager sLock;
};

inline BoltLockManager & BoltLockMgr(void)
{
    return BoltLockManager::sLock;
}

#endif // LOCK_MANAGER_H
