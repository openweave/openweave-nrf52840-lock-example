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

#ifndef LOCK_WIDGET_H
#define LOCK_WIDGET_H

#include <stdint.h>
#include <stdbool.h>

#include "AppEvent.h"

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

    static void TimerEventHandler(void * p_context);
    static void LockTimerEventHandler(AppEvent *aEvent);
};

#endif // LOCK_WIDGET_H
