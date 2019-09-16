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

#include "AppEvent.h"

int AppEventDispatcher::AddEventHandler(EventHandler Handler, intptr_t Arg = 0)
{
    int ret = NRF_SUCCESS;
    EventHandler_t * eventHandler;

    // Do nothing if the event handler is already registered.
    for (eventHandler = mAppEventHandlerList; eventHandler != NULL; eventHandler = eventHandler->Next)
    {
        if (eventHandler->Handler == Handler && eventHandler->Arg == Arg)
        {
            return ret;
        }
    }

    eventHandler = (EventHandler_t *)malloc(sizeof(EventHandler_t));
    if (eventHandler == NULL)
    {
        ret = NRF_ERROR_NO_MEM;
    }
    else
    {
        eventHandler->Next = mAppEventHandlerList;
        eventHandler->Handler = Handler;
        eventHandler->Arg = Arg;

        mAppEventHandlerList = eventHandler;
    }

    return ret;
}


void AppEventDispatcher::RemoveEventHandler(EventHandler Handler, intptr_t Arg = 0)
{
    EventHandler_t ** eventHandlerIndirectPtr;

    for (eventHandlerIndirectPtr = &mAppEventHandlerList; *eventHandlerIndirectPtr != NULL; )
    {
        EventHandler_t * eventHandler = (*eventHandlerIndirectPtr);

        if (eventHandler->Handler == Handler && eventHandler->Arg == Arg)
        {
            *eventHandlerIndirectPtr = eventHandler->Next;
            free(eventHandler);
        }
        else
        {
            eventHandlerIndirectPtr = &eventHandler->Next;
        }
    }
}


void AppEventDispatcher::DispatchEvent(const AppEvent * event)
{
    // Dispatch the event to each of the registered application event handlers.
    for (EventHandler_t * eventHandler = mAppEventHandlerList;
         eventHandler != NULL;
         eventHandler = eventHandler->Next)
    {
        eventHandler->Handler(event, eventHandler->Arg);
    }
}