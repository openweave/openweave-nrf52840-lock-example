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

#ifndef APP_EVENT_H
#define APP_EVENT_H

#include <stdlib.h>
#include "ButtonManager.h"

struct AppEvent
{
    enum AppEventTypes
    {
        kEventType_Button = 0,
        kEventType_Timer,
        kEventType_LockAction_Requested,
        kEventType_LockAction_Initiated,
        kEventType_LockAction_Completed,
        kEventType_LocalDeviceDiscovered,
        kEventType_AutoReLock_Evaluate,
        kEventType_Install,
    };

    uint16_t Type;

    union
    {
        struct
        {
            uint8_t PinNo;
            ButtonManager::ButtonAction Action;
        } ButtonEvent;
        struct
        {
            void * Context;
        } TimerEvent;
        struct
        {
            uint8_t Action;
            int32_t Actor;
        } LockAction_Requested, LockAction_Initiated;
        struct
        {
            uint8_t Action;
        } LockAction_Completed;
    };

    typedef void (*EventHandler)(const AppEvent *);
    EventHandler Handler;
};


class AppEventDispatcher
{

public:
    typedef void (*EventHandler)(const AppEvent *, intptr_t Arg);

    struct EventHandler_t
    {
        EventHandler_t * Next;
        EventHandler Handler;
        intptr_t Arg;
    };

    // API to add or remove a button event handler.
    int AddEventHandler(EventHandler Handler, intptr_t arg);
    void RemoveEventHandler(EventHandler Handler, intptr_t arg);

protected:
    void DispatchEvent(const AppEvent * aEvent);

    EventHandler_t * mAppEventHandlerList;

};

#endif // APP_EVENT_H
